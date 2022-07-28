#include "renderer.h"

static void mesh_on_load(const char* filename, u8* raw, usize raw_size, void* payload, usize payload_size, void* udata) {
	init_model_from_fbx(payload, raw, raw_size);
}

static void mesh_on_unload(void* payload, usize payload_size) {
	deinit_model(payload);
}

void register_renderer_resources() {
	reg_res_type("model", &(struct res_config) {
		.payload_size = sizeof(struct model),
		.free_raw_on_load = true,
		.terminate_raw = false,
		.on_load = mesh_on_load,
		.on_unload = mesh_on_unload
	});
}

struct renderer* new_renderer() {
	struct renderer* renderer = core_calloc(1, sizeof *renderer);

	return renderer;
}

void free_renderer(struct renderer* renderer) {
	free_vector(renderer->transforms);
	core_free(renderer);
}

void renderer_begin(struct renderer* renderer) {
	vector_clear(renderer->transforms);

	for (struct mesh** i = table_first(renderer->drawlist); i; i = table_next(renderer->drawlist, *i)) {
		usize* count = table_get(renderer->drawlist, *i);
		*count = 0;
	}
}

void renderer_end(struct renderer* renderer) {

}

void renderer_push(struct renderer* renderer, struct mesh* mesh, m4f transform) {
	usize* count = table_get(renderer->drawlist, mesh);
	if (!count) {
		table_set(renderer->drawlist, mesh, 0);
		count = table_get(renderer->drawlist, mesh);
	}

	*count += 1;

	vector_push(renderer->transforms, transform);
}
