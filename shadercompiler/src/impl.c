#include <corrosion/../../src/log.c>

u64 elf_hash(const u8* data, usize size) {
	usize hash = 0, x = 0;

	for (usize i = 0; i < size; i++) {
		hash = (hash << 4) + data[i];
		if ((x = hash & 0xF000000000LL) != 0) {
			hash ^= (x >> 24);
			hash &= ~x;
		}
	}

	return (hash & 0x7FFFFFFFFF);
}
