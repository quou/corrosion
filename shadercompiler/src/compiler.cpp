#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

extern "C" {
#include <corrosion/common.h>
#include <corrosion/core.h>
}

/* Usage: compiler file output */

struct PrepOut {
	bool is_compute;

	std::string vert_src;
	std::string frag_src;
	std::string comp_src;
};

#pragma pack(push, 1)
struct ShaderHeader {
	char header[2];
	u64 v_offset;
	u64 f_offset;
	u64 v_size;
	u64 f_size;
	u64 v_gl_offset;
	u64 f_gl_offset;
	u64 v_gl_size;
	u64 f_gl_size;
};
#pragma pack(pop)

static bool preprocess(const char* filepath, PrepOut& out) {
	out.is_compute = false;

	std::ifstream file(filepath);
	if (!file.good()) {
		error("Failed to open `%s'", filepath);
		return false;
	}

	std::string line;
	std::string version;

	i32 adding_to = -1;

	i32 c = 1;
	while (std::getline(file, line)) {
		if (c == 1) {
			if (line.find("#version") == 0) {
				version = line;
			} else {
				error("#version string must be on the first line.");
				return false;
			}
		}

		if (line.find("#compute") == 0) {
			out.is_compute = true;
			out.comp_src += std::to_string(c + 1) + "\n";
			goto loop_end;
		} else if (!out.is_compute && line.find("#begin") == 0) {
			if (line.find("vertex") != std::string::npos) {
				adding_to = 0;

				if (out.vert_src.size() == 0) {
					out.vert_src += version + "\n";
				}

				out.vert_src += "#line " + std::to_string(c + 1) + "\n";

				goto loop_end;
			} else if (line.find("fragment") != std::string::npos) {
				adding_to = 1;

				if (out.frag_src.size() == 0) {
					out.frag_src += version + "\n";
				}

				out.frag_src += "#line " + std::to_string(c + 1) + "\n";

				goto loop_end;
			} else {
				error("Expected `vertex' or `fragment' to follow #begin.");
				return false;
			}
		} else if (!out.is_compute && line.find("#end") == 0) {
			adding_to = -1;
		}

		if (out.is_compute) {
			out.comp_src += line + "\n";
		}

		switch (adding_to) {
			case 0:
				out.vert_src += line + "\n";
				break;
			case 1:
				out.frag_src += line + "\n";
				break;
			default: break;
		}

loop_end:

		c++;
	}

	return true;
}

static std::string compile_for_opengl(const std::vector<u32> data) {
	spirv_cross::CompilerGLSL compiler(data);

	spirv_cross::CompilerGLSL::Options options;

	options.version = 330;
	options.es = true;

	compiler.set_common_options(options);

	spirv_cross::ShaderResources resources = compiler.get_shader_resources();

	/* Modify bindings of uniforms and samplers, because OpenGL doesn't support descriptor sets */
	for (auto& resource : resources.sampled_images) {
		u32 set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
		u32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

		compiler.unset_decoration(resource.id, spv::DecorationDescriptorSet);

		/* TODO: Properly modify the binding. */
		u32 d;
		compiler.get_binary_offset_for_decoration(resource.id, spv::DecorationDescriptorSet, d);
		info("%d", d);

		compiler.set_decoration(resource.id, spv::DecorationBinding, binding);
	}

	return compiler.compile();
}

i32 main(i32 argc, const char** argv) {
	if (argc < 3) {
		error("Usage: %s file out", argv[0]);
		return 1;
	}

	PrepOut prep;
	if (!preprocess(argv[1], prep)) {
		return 1;
	}

	if (!prep.is_compute) {
		if (prep.vert_src.size() == 0 || prep.frag_src.size() == 0) {
			error("A shader must have both a vertex and fragment shader unless it is marked with #compute");
			return 1;
		}
	}

	shaderc::Compiler compiler;
	shaderc::CompileOptions options;
	options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
	options.SetOptimizationLevel(shaderc_optimization_level_performance);

	if (!prep.is_compute) {
		shaderc::SpvCompilationResult vertex_mod =
			compiler.CompileGlslToSpv(prep.vert_src.c_str(), shaderc_glsl_vertex_shader, argv[1], "main", options);
		if (vertex_mod.GetCompilationStatus() != shaderc_compilation_status_success) {
			std::stringstream ss(vertex_mod.GetErrorMessage());

			std::string error_line;
			while (std::getline(ss, error_line)) {
				error("%s", error_line.c_str());
			}

			return 1;
		}

		shaderc::SpvCompilationResult fragment_mod =
			compiler.CompileGlslToSpv(prep.frag_src.c_str(), shaderc_glsl_fragment_shader, argv[1], "main", options);
		if (fragment_mod.GetCompilationStatus() != shaderc_compilation_status_success) {
			std::stringstream ss(fragment_mod.GetErrorMessage());

			std::string error_line;
			while (std::getline(ss, error_line)) {
				error("%s", error_line.c_str());
			}

			return 1;
		}

		std::vector<u32> vert_data(vertex_mod.cbegin(), vertex_mod.cend());
		std::vector<u32> frag_data(fragment_mod.cbegin(), fragment_mod.cend());

		std::string vert_opengl_src = compile_for_opengl(vert_data);
		std::string frag_opengl_src = compile_for_opengl(frag_data);

		FILE* outfile = fopen(argv[2], "wb");
		if (!outfile) {
			error("Failed to fopen %s.", argv[2]);
			return 1;
		}

		ShaderHeader header{};
		header.header[0] = 'C';
		header.header[1] = 'S';
		header.v_size = vert_data.size() * sizeof(u32);
		header.f_size = frag_data.size() * sizeof(u32);
		header.v_offset = sizeof header;
		header.f_offset = header.v_offset + header.v_size;
		header.v_gl_size = vert_opengl_src.size() + 1;
		header.f_gl_size = frag_opengl_src.size() + 1;
		header.v_gl_offset = header.f_offset + header.f_size;
		header.f_gl_offset = header.v_gl_offset + header.v_gl_size;

		fwrite(&header, 1, sizeof header, outfile);
		fwrite(&vert_data[0], 1, header.v_size, outfile);
		fwrite(&frag_data[0], 1, header.f_size, outfile);

		fwrite(vert_opengl_src.c_str(), 1, vert_opengl_src.size(), outfile);
		fwrite("\0", 1, 1, outfile);
		fwrite(frag_opengl_src.c_str(), 1, frag_opengl_src.size(), outfile);
		fwrite("\0", 1, 1, outfile);

		fclose(outfile);
	} else {
		shaderc::SpvCompilationResult compute_mod =
			compiler.CompileGlslToSpv(prep.comp_src.c_str(), shaderc_glsl_compute_shader, argv[1], "main", options);
		if (compute_mod.GetCompilationStatus() != shaderc_compilation_status_success) {
			std::stringstream ss(compute_mod.GetErrorMessage());

			std::string error_line;
			while (std::getline(ss, error_line)) {
				error("%s", error_line.c_str());
			}

			return 1;
		}

		abort_with("Compute shader compilation not yet implemented.");
	}

	return 0;
}
