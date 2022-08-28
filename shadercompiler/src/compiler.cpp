#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>

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
struct ShaderRasterHeader {
	u64 v_offset;
	u64 f_offset;
	u64 v_size;
	u64 f_size;
	u64 v_gl_offset;
	u64 f_gl_offset;
	u64 v_gl_size;
	u64 f_gl_size;
};

struct ShaderComputeHeader {
	u64 offset;
	u64 size;
	u64 gl_offset;
	u64 gl_size;
};

struct ShaderHeader {
	char header[3];
	u8 is_compute;
	u64 bind_table_count;
	u64 bind_table_offset;

	union {
		ShaderRasterHeader raster_header;
		ShaderComputeHeader compute_header;
	};
};
#pragma pack(pop)

struct Desc {
	spirv_cross::CompilerGLSL& compiler;
	u32 binding;
	u64 id;
};

struct DescSet {
	u32 count = 0;

	std::vector<Desc> bindings;
};

struct DescID {
	u32 set;
	u32 binding;
};

/* Maps an ID optained from hashing a DescID to actual shader bindings, for OpenGL. */
std::unordered_map<u64, u32> set_bindings;
std::unordered_map<u32, DescSet> sets;

u32 current_binding = 0;

static std::string convert_filename(const std::string& path, const std::string& name) {
	usize slash = path.find_last_of("/");
	return path.substr(0, slash + 1) + name;
}

static void compute_set_bindings() {
	for (auto& sp : sets) {
		auto id = sp.first;
		auto& set = sp.second;

		for (auto& desc : set.bindings) {
			u32 binding = current_binding++;

			desc.compiler.set_decoration((spirv_cross::ID)desc.id, spv::DecorationBinding, binding);

			DescID did{ id, desc.binding };

			set_bindings[elf_hash(reinterpret_cast<u8*>(&did), sizeof did)] = binding;
		}
	}
}

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
			out.comp_src += version + "\n";
			out.comp_src += "#line " + std::to_string(c + 1) + "\n";
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

static void compile_for_opengl(spirv_cross::CompilerGLSL& compiler) {
	spirv_cross::CompilerGLSL::Options options;

	options.version = 320;
	options.es = true;

	compiler.set_common_options(options);

	spirv_cross::ShaderResources resources = compiler.get_shader_resources();

	/* Modify bindings of uniforms and samplers, because OpenGL doesn't support descriptor sets */
	for (auto& resource : resources.sampled_images) {
		u32 set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
		u32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

		sets[set].count++;
		sets[set].bindings.push_back(Desc { compiler, binding, resource.id });

		compiler.unset_decoration(resource.id, spv::DecorationDescriptorSet);
	}

	for (auto& resource : resources.uniform_buffers) {
		u32 set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
		u32 binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

		sets[set].count++;
		sets[set].bindings.push_back(Desc { compiler, binding, resource.id });

		compiler.unset_decoration(resource.id, spv::DecorationDescriptorSet);
	}
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

		spirv_cross::CompilerGLSL v_compiler(vert_data);
		spirv_cross::CompilerGLSL f_compiler(frag_data);

		compile_for_opengl(v_compiler);
		compile_for_opengl(f_compiler);

		compute_set_bindings();
		
		std::string vert_opengl_src = v_compiler.compile();
		std::string frag_opengl_src = f_compiler.compile();

		FILE* outfile = fopen(argv[2], "wb");
		if (!outfile) {
			error("Failed to fopen %s.", argv[2]);
			return 1;
		}

		ShaderHeader header{};
		header.header[0] = 'C';
		header.header[1] = 'S';
		header.header[2] = 'H';

		header.bind_table_count = 0;
		for (const auto& sbp : set_bindings) {
			header.bind_table_count++;
		}

		header.is_compute = 0;

		ShaderRasterHeader& r_header = header.raster_header;

		r_header.v_size = vert_data.size() * sizeof(u32);
		r_header.f_size = frag_data.size() * sizeof(u32);
		r_header.v_offset = sizeof header;
		r_header.f_offset = r_header.v_offset + r_header.v_size;
		r_header.v_gl_size = vert_opengl_src.size() + 1;
		r_header.f_gl_size = frag_opengl_src.size() + 1;
		r_header.v_gl_offset = r_header.f_offset + r_header.f_size;
		r_header.f_gl_offset = r_header.v_gl_offset + r_header.v_gl_size;

		header.bind_table_offset = r_header.f_gl_offset + r_header.f_gl_size;

		fwrite(&header, 1, sizeof header, outfile);
		fwrite(&vert_data[0], 1, r_header.v_size, outfile);
		fwrite(&frag_data[0], 1, r_header.f_size, outfile);

		fwrite(vert_opengl_src.c_str(), 1, vert_opengl_src.size(), outfile);
		fwrite("\0", 1, 1, outfile);
		fwrite(frag_opengl_src.c_str(), 1, frag_opengl_src.size(), outfile);
		fwrite("\0", 1, 1, outfile);

		/* Write the binding table, for the OpenGL backend. */
		for (const auto& sbp : set_bindings) {
			fwrite(&sbp.first, 1, sizeof sbp.first, outfile);
			fwrite(&sbp.second, 1, sizeof sbp.second, outfile);
		}

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

		std::vector<u32> com_data(compute_mod.cbegin(), compute_mod.cend());

		spirv_cross::CompilerGLSL gl_compiler(com_data);

		compile_for_opengl(gl_compiler);

		compute_set_bindings();

		auto gl_src = gl_compiler.compile();

		FILE* outfile = fopen(argv[2], "wb");

		ShaderHeader header;
		header.header[0] = 'C';
		header.header[1] = 'S';
		header.header[2] = 'H';

		header.bind_table_count = 0;
		for (const auto& sbp : set_bindings) {
			header.bind_table_count++;
		}

		header.is_compute = 1;

		ShaderComputeHeader& c_header = header.compute_header;

		c_header.size = com_data.size() * sizeof(u32);
		c_header.offset = sizeof header;
		c_header.gl_size = gl_src.size() + 1;
		c_header.gl_offset = c_header.offset + c_header.size;

		header.bind_table_offset = c_header.gl_offset + c_header.gl_size;

		fwrite(&header, 1, sizeof header, outfile);
		fwrite(&com_data[0], 1, com_data.size() * sizeof(u32), outfile);
		fwrite(gl_src.c_str(), 1, gl_src.size() + 1, outfile);

		for (const auto& sbp : set_bindings) {
			fwrite(&sbp.first, 1, sizeof sbp.first, outfile);
			fwrite(&sbp.second, 1, sizeof sbp.second, outfile);
		}

		fclose(outfile);
	}

	return 0;
}
