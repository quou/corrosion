#include "video_internal.h"

void copy_pipeline_descriptor(struct pipeline_descriptor* dst, const struct pipeline_descriptor* src) {
	dst->name     = copy_string(src->name);
	dst->binding  = src->binding;
	dst->stage    = src->stage;
	dst->resource = src->resource;
}

void copy_pipeline_descriptor_set(struct pipeline_descriptor_set* dst, const struct pipeline_descriptor_set* src) {
	dst->name        = copy_string(src->name);
	dst->count       = src->count;
	dst->descriptors = null;
	vector(struct pipeline_descriptor) descs_v = null;
	vector_allocate(descs_v, src->count);

	for (usize ii = 0; ii < src->count; ii++) {
		struct pipeline_descriptor desc = { 0 };
		const struct pipeline_descriptor* other_desc = src->descriptors + ii;

		copy_pipeline_descriptor(&desc, other_desc);

		vector_push(descs_v, desc);
	}

	dst->descriptors = descs_v;
}
