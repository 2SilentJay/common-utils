#ifndef INTRUSIVEMAP_H
#define INTRUSIVEMAP_H

#include <memory>

#include "../Log.h"

template <typename K, typename V>
class IntrusiveMap {
public:
	struct Bucket {
		V* list;
		size_t size;

		Bucket() noexcept : list(nullptr), size(0) { }
	};
	
	struct Hook {
		V* im_next;
		K im_key;
		bool im_linked;

		Hook() noexcept : im_next(nullptr), im_key(), im_linked(false) { }
	};
	
	const size_t bucket_list_size;
	
private:
	Bucket * const bucket_list;
	size_t elements;

	template<typename ITV>
	struct Iterator {
		friend class IntrusiveMap;

		Iterator() noexcept : value(nullptr) { }

		Iterator(ITV* value) noexcept : value(value) { }

		bool operator==(const Iterator& it) const noexcept {
			return value == it.value;
		}

		bool operator!=(const Iterator& it) const noexcept {
			return value != it.value;
		}

		Iterator& operator++() noexcept {
			value = value->im_next;
			return *this;
		}

		Iterator operator++(int) noexcept {
			value = value->im_next;
			return Iterator(value);
		}

		ITV& operator*() noexcept {
			return *value;
		}

		ITV* operator->() noexcept {
			return value;
		}

		const ITV& operator*() const noexcept {
			return *value;
		}

		const ITV* operator->() const noexcept {
			return value;
		}

	private:
		ITV* value;
	};

	typedef Iterator<V> Iterator_t;
	typedef Iterator<const V> ConstIterator_t;
	
public:

	IntrusiveMap(Bucket* bucket_list, size_t bucket_list_size) noexcept :
	bucket_list_size(bucket_list_size),
	bucket_list(bucket_list),
	elements(0) {}

	IntrusiveMap(const IntrusiveMap&) = delete;
	IntrusiveMap(IntrusiveMap&&) = delete;

	IntrusiveMap& operator=(const IntrusiveMap&) = delete;
	IntrusiveMap& operator=(IntrusiveMap&&) = delete;

	~IntrusiveMap() noexcept = default;
	
	bool put(K key, V& value) noexcept {
		if (sanity_check(value)) {
			size_t index = key % bucket_list_size;
			if(find(bucket_list[index], key) == nullptr){
				link_front(bucket_list[index], key, value);
				return true;
			}
		}
		return false;
	}
	
	bool remove(K key) noexcept {
		size_t index = key % bucket_list_size;
		Bucket& bucket = bucket_list[index];
		V* prev = nullptr;
		V* result = find(bucket, key, prev);
		if(result){
			if(result == bucket.list)
				unlink_front(bucket);
			else
				unlink_next(bucket, *prev);
			return true;
		}
		return false;
	}
	
	void reset() noexcept {
		for (size_t i = 0; i < bucket_list_size; i++) {
			while(bucket_list[i].list)
				unlink_front(bucket_list[i]);
		}
	}
	
	V* find(K key) noexcept {
		size_t index = key % bucket_list_size;
		return find(bucket_list[index], key);
	}
	
	size_t size() const noexcept {
		return elements;
	}
	
	Iterator_t begin(size_t bucket) noexcept {
		return Iterator_t(bucket_list[bucket].list);
	}

	ConstIterator_t cbegin(size_t bucket) const noexcept {
		return ConstIterator_t(bucket_list[bucket].list);
	}

	Iterator_t end() noexcept {
		return Iterator_t();
	}

	ConstIterator_t cend() const noexcept {
		return ConstIterator_t();
	}

private:

	inline static bool sanity_check(const V& value) noexcept {
		return (not value.im_linked);
	}

	inline void link_front(Bucket& bucket, K key, V& value) noexcept {
		value.im_next = bucket.list;
		value.im_linked = true;
		value.im_key = key;
		bucket.list = &value;
		bucket.size++;
		elements++;
	}
	
	inline void unlink_front(Bucket& bucket) noexcept {
		V* tmp_value = bucket.list;
		bucket.list = bucket.list->im_next;
		tmp_value->im_next = nullptr;
		tmp_value->im_linked = false;
		bucket.size--;
		elements--;
	}
	
	inline void unlink_next(Bucket& bucket, V& value) noexcept {
		V* tmp_value = value.im_next;
		value.im_next = value.im_next->im_next;
		tmp_value->im_next = nullptr;
		tmp_value->im_linked = false;
		bucket.size--;
		elements--;
	}
	
	inline V* find(Bucket& bucket, K key) noexcept {
		V* cur = bucket.list;
		while(cur){
			if(cur->im_key == key)
				break;
			cur = cur->im_next;
		}
		return cur;
	}
	
	inline V* find(Bucket& bucket, K key, V*& prev) noexcept {
		V* cur = bucket.list;
		while(cur){
			if(cur->im_key == key)
				break;
			prev = cur;
			cur = cur->im_next;
		}
		return cur;
	}

};

#endif /* INTRUSIVEMAP_H */

