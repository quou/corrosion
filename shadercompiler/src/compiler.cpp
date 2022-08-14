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
struct ShaderHeader {
	char header[2];
	u16 bind_table_count;
	u64 v_offset;
	u64 f_offset;
	u64 v_size;
	u64 f_size;
	u64 v_gl_offset;
	u64 f_gl_offset;
	u64 v_gl_size;
	u64 f_gl_size;
	u64 bind_table_offset;
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

std::vector<std::string> included;

static std::string convert_filename(const std::string& path, const std::string& name) {
	usize slash = path.find_last_of("/");
	return path.substr(0, slash + 1) + name;
}

static bool include_file(const std::string& filepath, std::string& out) {
	std::ifstream file(filepath);
	if (!file.good()) {
		error("Failed to open `%s'", filepath.c_str());
		return false;
	}

	std::string line;

	while (std::getline(file, line)) {
		if (line.find("#include") == 0) {
			usize pos;
			if ((pos = line.find("\"")) != std::string::npos) {
				usize last_pos = line.find("\"", pos + 1);

				if (last_pos == pos || last_pos == std::string::npos) {
					error("Expected \" after string literal.");
					return false;
				}

				std::string i_filename = convert_filename(filepath, line.substr(pos + 1, last_pos - pos - 1));

				if (std::find(included.begin(), included.end(), i_filename) == included.end()) {
					if (!include_file(i_filename, out)) {
						return false;
					}
				}
			} else {
				error("Expected string literal after `#include'.");
				return false;
			}
		} else {
			out += line;
		}
	}

	included.push_back(filepath);

	return true;
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
		} else if (line.find("#include") == 0) {
			usize pos;
			if ((pos = line.find("\"")) != std::string::npos) {
				usize last_pos = line.find("\"", pos + 1);

				if (last_pos == pos || last_pos == std::string::npos) {
					error("Expected \" after string literal.");
					return false;
				}

				std::string i_filename = convert_filename(filepath, line.substr(pos + 1, last_pos - pos - 1));

				if (std::find(included.begin(), included.end(), i_filename) == included.end()) {
					std::string* add_to = nullptr;

					if (adding_to == 0) {
						add_to = &out.vert_src;
					} else if (adding_to == 1) {
						add_to = &out.frag_src;
					}

					if (add_to) {
						if (!include_file(i_filename, *add_to)) {
							return false;
						}
					}
				}
			} else {
				error("Expected string literal after `#include'.");
				return false;
			}

			goto loop_end;
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

		for (auto& sp : sets) {
			auto id = sp.first;
			auto& set = sp.second;

			for (auto& desc : set.bindings) {
				u32 binding = current_binding++;

				desc.compiler.set_decoration(desc.id, spv::DecorationBinding, binding);

				DescID did{ id, desc.binding };

				set_bindings[elf_hash(reinterpret_cast<u8*>(&did), sizeof did)] = binding;
			}
		}
		
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

		header.bind_table_count = 0;
		for (const auto& sbp : set_bindings) {
			header.bind_table_count++;
		}

		header.v_size = vert_data.size() * sizeof(u32);
		header.f_size = frag_data.size() * sizeof(u32);
		header.v_offset = sizeof header;
		header.f_offset = header.v_offset + header.v_size;
		header.v_gl_size = vert_opengl_src.size() + 1;
		header.f_gl_size = frag_opengl_src.size() + 1;
		header.v_gl_offset = header.f_offset + header.f_size;
		header.f_gl_offset = header.v_gl_offset + header.v_gl_size;
		header.bind_table_offset = header.f_gl_offset + header.f_gl_size;

		fwrite(&header, 1, sizeof header, outfile);
		fwrite(&vert_data[0], 1, header.v_size, outfile);
		fwrite(&frag_data[0], 1, header.f_size, outfile);

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

		abort_with("Compute shader compilation not yet implemented.");
	}

	return 0;
}
