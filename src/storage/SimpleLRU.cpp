#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value)
{
    auto it = _lru_index.find(std::ref(key));

    if (it == _lru_index.end()) {
        if (!check_size(key.size() + value.size())) { return false; }
        lru_node *new_node = put_node(key, value);
        if (new_node == nullptr ) { return false; }
        _lru_index.insert(std::make_pair(std::ref(new_node->key), std::ref(*new_node)));
    } else {
        move_node_to_head(it->second);
        _cur_size -= it->second.get().value.size() + it->second.get().key.size();
        if (!check_size(value.size() + it->second.get().key.size())) {
            _cur_size += it->second.get().value.size() + it->second.get().key.size();
            return false;
        }
        _cur_size = value.size() + it->second.get().key.size();
        it->second.get().value = value;
    }
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value)
{
    auto it = _lru_index.find(std::ref(key));
    if (it != _lru_index.end()) { return false; }
    if (!check_size(key.size() + value.size())) { return false; }
    lru_node *new_node = put_node(key, value);
    _lru_index.insert(std::make_pair(std::ref(new_node->key), std::ref(*new_node)));
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value)
{
    auto it = _lru_index.find(std::ref(key));
    if (it == _lru_index.end()) { return false; }
    move_node_to_head(it->second.get());
    if (!check_size(value.size() - it->second.get().value.size())) { return false; }
    it->second.get().value = value;
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key)
{
    auto it = _lru_index.find(std::ref(key));
    if (it == _lru_index.end()) { return false; }
    remove_node(it->second.get());
    _lru_index.erase(it);
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value)
{
    auto it = _lru_index.find(std::ref(key));
    if (it == _lru_index.end()) { return false; }
    value = it->second.get().value;
    return true;
}

} // namespace Backend
} // namespace Afina
