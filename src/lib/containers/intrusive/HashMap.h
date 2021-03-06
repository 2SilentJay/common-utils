#ifndef INTRUSIVE_HASHMAP_H
#define INTRUSIVE_HASHMAP_H

#include <memory>
#include <cassert>

namespace intrusive {

template<typename K, typename V>
struct HashMapHook {
	V* im_next;
	K im_key;
	bool im_linked;

	HashMapHook() noexcept : im_next(nullptr), im_key(), im_linked(false) {}

	HashMapHook(const HashMapHook&) = delete;
	HashMapHook& operator=(const HashMapHook&) = delete;

	HashMapHook(HashMapHook&&) = delete;
	HashMapHook& operator=(HashMapHook&&) = delete;
};

template<typename MapData_t>
struct HashMapBucket {
	MapData_t* head;
	size_t size;

	HashMapBucket() noexcept : head(nullptr), size(0) {}

	HashMapBucket(const HashMapBucket&) = delete;
	HashMapBucket& operator=(const HashMapBucket&) = delete;

	HashMapBucket(HashMapBucket&&) = delete;
	HashMapBucket& operator=(HashMapBucket&&) = delete;
};

/**
 * An unordered hash map implemented in an intrusive way.
 * Can hold many items for one key.
 */

template<typename K, typename MapNode, typename H = std::hash<K>, typename A = std::allocator<HashMapBucket<MapNode> > >
class HashMap {
public:
	using Bucket_t = HashMapBucket<MapNode>;

private:
	Bucket_t* bucket_list;
	size_t bucket_list_size;
	size_t elements;
	H hasher;
	A allocator;

	template<typename N>
	struct Iterator {
		friend class HashMap;

		Iterator() noexcept : m_node(nullptr) {}

		Iterator(N* node) noexcept : m_node(node) {}

		inline bool operator==(const Iterator& it) const noexcept {
			return m_node == it.m_node;
		}

		inline bool operator!=(const Iterator& it) const noexcept {
			return m_node != it.m_node;
		}

		inline Iterator& operator++() noexcept {
			m_node = m_node->im_next;
			return *this;
		}

		inline Iterator operator++(int)noexcept {
			m_node = m_node->im_next;
			return Iterator(m_node);
		}

		inline Iterator& next(const K& key) noexcept {
			m_node = m_node->im_next;
			while(m_node) {
				if(m_node->im_key == key) {
					break;
				}
				m_node = m_node->im_next;
			}
			return *this;
		}

		inline const N& operator*() const noexcept {
			return *m_node;
		}

		inline N& operator*() noexcept {
			return *m_node;
		}

		inline const N* operator->() const noexcept {
			return m_node;
		}

		inline N* operator->() noexcept {
			return m_node;
		}

		inline const K& key() noexcept {
			return m_node->im_key;
		}

		inline operator bool() const noexcept {
			return m_node;
		}

		inline const N* get() const noexcept {
			return m_node;
		}

		inline N* get() noexcept {
			return m_node;
		}

	private:
		N* m_node;
	};

public:

	using Iterator_t = Iterator<MapNode>;
	using ConstIterator_t = Iterator<const MapNode>;

	HashMap(size_t bucket_list_size) noexcept :
		bucket_list(nullptr), bucket_list_size(bucket_list_size), elements(0), hasher(), allocator() {}

	HashMap(const HashMap&) = delete;
	HashMap& operator=(const HashMap&) = delete;

	HashMap(HashMap&& rv) noexcept :
		bucket_list(rv.bucket_list)
		, bucket_list_size(rv.bucket_list_size)
		, elements(rv.elements)
		, hasher(rv.hasher)
		, allocator(rv.allocator) {
		rv.bucket_list = nullptr;
		rv.destroy();
	}

	HashMap& operator=(HashMap&& rv) noexcept {
		if(this != &rv) {
			destroy();
			bucket_list = rv.bucket_list;
			bucket_list_size = rv.bucket_list_size;
			elements = rv.elements;
			allocator = rv.allocator;
			hasher = rv.hasher;
			rv.clean_state();
		}
		return *this;
	}

	/**
	 * Be careful, The map must be empty before the storage has been destroyed.
	 * That means the method 'clean()' must be called before destroying the storage.
	 */
	virtual ~HashMap() noexcept {
		destroy();
	}

	/**
	 * Allocate the bucket storage of the map.
	 * @return true - if the bucket storage has been allocated successfully.
	 */
	bool allocate() noexcept {
		if(bucket_list)
			return false;

		bucket_list = allocator.allocate(bucket_list_size);
		if(bucket_list) {
			for(size_t i = 0; i < bucket_list_size; i++) {
				allocator.construct(bucket_list + i);
			}
		}
		return bucket_list != nullptr;
	}

	/**
	 * Unlink all the objects the map contains.
	 */
	void clear() noexcept {
		for(size_t i = 0; i < bucket_list_size; i++) {
			while(bucket_list[i].head)
				unlink_front(i);
		}
	}

	/**
	 * Link a key with a node.
	 * The node must not be linked.
	 * @param key
	 * @param node
	 * @return 
	 */
	Iterator_t link(const K& key, MapNode& node) noexcept {
		check_free(node); // TODO: debug
		size_t bucket_id = hasher(key) % bucket_list_size;
		link_front(bucket_id, key, node);
		return Iterator_t(&node);
	}

	/**
	 * Find the first node which is linked to the key.
	 * @param key
	 * @return 
	 */
	ConstIterator_t find(const K& key) const noexcept {
		size_t bucket_id = hasher(key) % bucket_list_size;
		return ConstIterator_t(find(bucket_id, key));
	}

	/**
	 * Find the first node which is linked to the key.
	 * @param key
	 * @return 
	 */
	Iterator_t find(const K& key) noexcept {
		size_t bucket_id = hasher(key) % bucket_list_size;
		return Iterator_t(find(bucket_id, key));
	}

	/**
	 * Remove the node.
	 * The node must be linked.
	 * @param key
	 * @return 
	 */
	void remove(MapNode& node) noexcept {
		check_linked(node); // TODO: debug
		size_t bucket_id = hasher(node.im_key) % bucket_list_size;
		if(&node == bucket_list[bucket_id].head) {
			unlink_front(bucket_id);
		} else {
			MapNode* prev = find_prev(bucket_id, &node);
			unlink_next(bucket_id, *prev);
		}
	}

	/**
	 * Remove the node with an iterator.
	 * The node must be linked.
	 * @param key
	 * @return 
	 */
	void remove(Iterator_t it) noexcept {
		remove(*it);
	}

	/**
	 * @return amount of currently linked nodes.
	 */
	inline size_t size() const noexcept {
		return elements;
	}

	/**
	 * @return amount of the map buckets.
	 */
	inline size_t buckets() const noexcept {
		return bucket_list_size;
	}

	inline Iterator_t begin(size_t bucket) noexcept {
		return Iterator_t(bucket_list[bucket].head, bucket);
	}

	inline ConstIterator_t cbegin(size_t bucket) const noexcept {
		return ConstIterator_t(bucket_list[bucket].head);
	}

	inline Iterator_t end() noexcept {
		return Iterator_t();
	}

	inline ConstIterator_t cend() const noexcept {
		return ConstIterator_t();
	}

private:

	void destroy() noexcept {
		if(bucket_list) {
			clear();
			for(size_t i = 0; i < bucket_list_size; i++) {
				allocator.destroy(bucket_list + i);
			}
			allocator.deallocate(bucket_list, bucket_list_size);
		}
		clean_state();
	}

	inline static void check_free(const MapNode& node) noexcept {
		assert(not node.im_linked);
	}

	inline static void check_linked(const MapNode& node) noexcept {
		assert(node.im_linked);
	}

	inline void link_front(size_t bucket_id, const K& key, MapNode& node) noexcept {
		Bucket_t& bucket = bucket_list[bucket_id];
		node.im_next = bucket.head;
		node.im_linked = true;
		node.im_key = key;
		bucket.head = &node;
		bucket.size++;
		elements++;
	}

	inline void unlink_front(size_t bucket_id) noexcept {
		Bucket_t& bucket = bucket_list[bucket_id];
		MapNode* tmp_value = bucket.head;
		bucket.head = bucket.head->im_next;
		tmp_value->im_next = nullptr;
		tmp_value->im_linked = false;
		bucket.size--;
		elements--;
	}

	inline void unlink_next(size_t bucket_id, MapNode& node) noexcept {
		Bucket_t& bucket = bucket_list[bucket_id];
		MapNode* tmp_value = node.im_next;
		node.im_next = node.im_next->im_next;
		tmp_value->im_next = nullptr;
		tmp_value->im_linked = false;
		bucket.size--;
		elements--;
	}

	inline MapNode* find(size_t bucket_id, const K& key) noexcept {
		MapNode* cur = bucket_list[bucket_id].head;
		while(cur) {
			if(cur->im_key == key)
				break;
			cur = cur->im_next;
		}
		return cur;
	}

	inline const MapNode* find(size_t bucket_id, const K& key) const noexcept {
		MapNode* cur = bucket_list[bucket_id].head;
		while(cur) {
			if(cur->im_key == key) {
				break;
			}
			cur = cur->im_next;
		}
		return cur;
	}

	inline MapNode* find_prev(size_t bucket_id, const MapNode* node) const noexcept {
		MapNode* cur = bucket_list[bucket_id].head;
		MapNode* prev = nullptr;
		while(cur) {
			if(cur == node) {
				return prev;
			}
			prev = cur;
			cur = cur->im_next;
		}
		return nullptr;
	}

	inline void clean_state() noexcept {
		bucket_list = nullptr;
		bucket_list_size = 0;
		elements = 0;
	}

};

}; // namespace intrusive

#endif /* INTRUSIVE_HASHMAP_H */

