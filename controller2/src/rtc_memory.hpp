extern "C" {
#include <ets_sys.h>
#include <osapi.h>
#include <user_interface.h>
void os_delay_us(uint32_t);
};

template <class T>
class rtc_memory {
	// 4 bytes aligned memory block address in rtc memory.
	// user data must use 64 or larger block.
	// but system_rtc_mem_read() is failed for 64 so use 65.
	static constexpr uint32_t USER_DATA_ADDR = 65;
	static constexpr uint32_t USER_DATA_SIZE = 512 - ((USER_DATA_ADDR - 64) * 4);

	static uint32_t fnv_1_hash_32(uint8_t *bytes, size_t length) {
		static const uint32_t FNV_OFFSET_BASIS_32 = 2166136261U;
		static const uint32_t FNV_PRIME_32 = 16777619U;
		uint32_t hash = FNV_OFFSET_BASIS_32;
		for (size_t i = 0 ; i < length ; ++i) hash = (FNV_PRIME_32 * hash) ^ (bytes[i]);
		return hash;
	}

	uint32_t calc_hash(T& data) const {
		return fnv_1_hash_32((uint8_t*)&data, sizeof(data));
	}

public:
	uint32_t hash;
	T data;
	static_assert(sizeof(T) <= (USER_DATA_SIZE - sizeof(hash)), "sizeof(T) it too big");

	bool read() {
		// Read memory to temporary variable to retain initial values in struct T.
		rtc_memory<T> read;

		// An initial rtc memory is random.
		bool ok = system_rtc_mem_read(USER_DATA_ADDR, &read, sizeof(read));
		if (ok) {
			// Only hashes are matched and copy to struct.
			if (read.hash == calc_hash(read.data)) {
				memcpy(this, &read, sizeof(read));
			}
		}
		return ok;
	}

	bool write() {
		hash = calc_hash(data);
		return system_rtc_mem_write(USER_DATA_ADDR, this, sizeof(*this));
	}

	T* operator->() {
		return &data;
	}
};
//
//
//template <class T>
//struct _rtc_memory_with_time {
//	uint32_t time_base = 0;
//	uint64_t time_acc = 0;
//	T data;
//};
//
//template <class T>
//class rtc_memory_with_time : public rtc_memory<_rtc_memory_with_time<T>> {
//	using super = rtc_memory<_rtc_memory_with_time<T>>;
//public:
//	T* operator->() {
//		// return &(this->data.data);
//		return &(super::data.data);
//	}
//
//	bool init() {
//		bool ok;
//		ok = super::read();
//		if (!ok) return ok;
//
//		if (!super::data.time_base) {
//			Serial.println("rtc time init...");
//			super::data.time_acc = 0;
//			super::data.time_base = system_get_rtc_time();
//		}
//		Serial.print("time base = ");
//		Serial.println(super::data.time_base);
//
//		const uint32_t rtc_t1 = system_get_rtc_time();
//		const uint32_t st1 = system_get_time();
//		const uint32_t cal1 = system_rtc_clock_cali_proc();
//		os_delay_us(300);
//		const uint32_t rtc_t2 = system_get_rtc_time();
//		const uint32_t st2 = system_get_time();
//		const uint32_t cal2 = system_rtc_clock_cali_proc();
//
//		Serial.print("rtc_t1 = ");
//		Serial.println(rtc_t1);
//		Serial.print("rtc_t2 = ");
//		Serial.println(rtc_t2);
//		Serial.print("rtc_t2 - super::data.time_base = ");
//		Serial.println(rtc_t2 - super::data.time_base);
//		Serial.print("cal1 = ");
//		Serial.println((cal1 * 1000)>>12);
//		Serial.print("cal2 = ");
//		Serial.println((cal2 * 1000)>>12);
//
//		super::data.time_acc += (((uint64)(rtc_t2 - super::data.time_base)) * ( (uint64)((cal2*1000)>>12))) / 1000;
//		super::data.time_base = rtc_t2;
//		Serial.print("new time base = ");
//		Serial.println(super::data.time_base);
//
//		super::write();
//
//		return 1;
//	}
//
//	uint64_t get_time_us() {
//		return system_get_time() + super::data.time_acc;
//	}
//
//	uint64_t get_time_ms() {
//		return get_time_us() / 1000;
//	}
//
//	uint64_t get_time_s() {
//		return get_time_us() / 1000 / 1000;
//	}
//};
