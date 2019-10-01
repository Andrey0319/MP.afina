#ifndef AFINA_STORAGE_SIMPLE_LRU_H
#define AFINA_STORAGE_SIMPLE_LRU_H

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <afina/Storage.h>

namespace Afina {
namespace Backend {

/**
 * # Map based implementation
 * That is NOT thread safe implementaiton!!
 */
class SimpleLRU : public Afina::Storage {
public:
    SimpleLRU(size_t max_size = 1024)
    {
        _max_size = max_size;
        _cur_size = 0;
        _lru_head = std::make_unique <lru_node>("", "");
        _lru_head->next = std::make_unique <lru_node>("", "");
        _lru_end = _lru_head->next.get();
        _lru_end->prev = _lru_head.get();
    }

    ~SimpleLRU() {
        _lru_index.clear();
        while(_lru_head) {_lru_head = std::move(_lru_head->next); }
    }

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value) override;

private:
    // LRU cache node
    using lru_node = struct lru_node {
        const std::string key;
        std::string value;
        lru_node *prev;
        std::unique_ptr<lru_node> next;

        lru_node(const std::string &key, const std::string &value, lru_node *prev = nullptr,
                 std::unique_ptr<lru_node> next = nullptr):
                key(key),
                value(value),
                prev(prev),
                next(std::move(next))
        {};
    };

    // Maximum number of bytes could be stored in this cache.
    // i.e all (keys+values) must be less the _max_size
    std::size_t _max_size;
    // Current size
    std::size_t _cur_size;

    // Main storage of lru_nodes, elements in this list ordered descending by "freshness": in the head
    // element that wasn't used for longest time.
    //
    // List owns all nodes
    std::unique_ptr<lru_node> _lru_head;
    lru_node *_lru_end;


    // Index of nodes from list above, allows fast random access to elements by lru_node#key
    std::map<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>, std::less<std::string>> _lru_index;

    // Change size of list:
    // Return true if putting new node is possible
    bool check_size(std::size_t elem_size)
    {
        if (elem_size > _max_size) { return false; }

        while (elem_size + _cur_size > _max_size) {
            if (_lru_head->next.get() == _lru_end) { return false; }
            _cur_size -= _lru_end->prev->key.size() + _lru_end->prev->value.size();
            auto it = _lru_index.find(std::ref(_lru_end->prev->key));
            _lru_index.erase(it);
            _lru_end->prev = _lru_end->prev->prev;
            _lru_end->prev->next = std::move(_lru_end->prev->next->next);
        }
        return true;
    }

    // Put new node to the list of nodes
    lru_node *put_node(const std::string &key, const std::string &value)
    {
        std::unique_ptr<lru_node> cur_node = std::make_unique<lru_node>(key, value);
        _cur_size += cur_node->key.size() + cur_node->value.size();
        cur_node->prev = _lru_head.get();
        cur_node->next = std::move(_lru_head->next);
        cur_node->next->prev = cur_node.get();
        _lru_head->next = std::move(cur_node);
        return _lru_head->next.get();
    }

    // Delete node
    void remove_node(std::reference_wrapper<lru_node> node)
    {
        _cur_size -= node.get().value.size() + node.get().key.size();
        node.get().next->prev = node.get().prev;
        node.get().prev->next = std::move(node.get().next);
    }

    // Move node to the head of list
    void move_node_to_head(std::reference_wrapper<lru_node> node)
    {
        std::unique_ptr<lru_node> cur_node;
        cur_node = std::move(node.get().prev->next);
        node.get().next->prev = node.get().prev;
        node.get().prev->next = std::move(node.get().next);
        cur_node->next = std::move(_lru_head->next);
        cur_node->prev = _lru_head.get();
        cur_node->next->prev = cur_node.get();
        _lru_head->next = std::move(cur_node);
    }

};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_SIMPLE_LRU_H
