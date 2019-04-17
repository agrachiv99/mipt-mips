/**
 * CacheReplacement.cpp
 * Cache replacement algorithms module.
 * @author Oleg Ladin, Denis Los, Andrey Agrachev
 */

#include "infra/replacement/cache_replacement.h"
#include <tree.h>

#include <list>
#include <unordered_map>
#include <cassert>
#include <cmath>

class LRUCacheInfo : public CacheReplacementInterface
{
    public:
        explicit LRUCacheInfo( std::size_t ways);

        void touch( std::size_t way) override ;
        void set_to_erase( std::size_t way) override ;
        void allocate( std::size_t way) override ;
        std::size_t update() override ;
        std::size_t get_ways() const override { return ways; }

    private:
        std::list<std::size_t> lru_list{};
        std::unordered_map<std::size_t, decltype(lru_list.cbegin())> lru_hash{};
        const std::size_t ways;
        void erase_lru_element();
};

LRUCacheInfo::LRUCacheInfo( std::size_t ways)
    : ways( ways)
{
    assert( ways != 0u);
    lru_hash.reserve( ways);

    for ( std::size_t i = 0; i < ways; i++)
    {
        lru_list.push_front( i);
        lru_hash.emplace( i, lru_list.begin());
    }
}

void LRUCacheInfo::touch( std::size_t way)
{
    const auto lru_it = lru_hash.find( way);
    assert( lru_it != lru_hash.end());
    // Put the way to the head of the list
    lru_list.splice( lru_list.begin(), lru_list, lru_it->second);
}

void LRUCacheInfo::set_to_erase( std::size_t way)
{
    const auto lru_it = lru_hash.find( way);
    assert( lru_it != lru_hash.end());
    lru_list.splice( lru_list.end(), lru_list, lru_it->second);
}

std::size_t LRUCacheInfo::update()
{
    // remove the least recently used element from the tail
    std::size_t lru_elem = lru_list.back();
    lru_list.pop_back();

    // put it to the head
    auto ptr = lru_list.insert( lru_list.begin(), lru_elem);
    lru_hash.insert_or_assign( lru_elem, ptr);

    return lru_elem;
}

void LRUCacheInfo::allocate( std::size_t way)
{
    if ( lru_hash.size() >= ways)
        erase_lru_element();
    lru_list.push_front( way);
    lru_hash.emplace( way, lru_list.begin());
}

void LRUCacheInfo::erase_lru_element() {
    std::size_t lru_elem = lru_list.back();
    lru_list.pop_back();
    lru_hash.erase( lru_elem);
}

///////////////////////////////////////////////////////////////////

struct node
{
    node(int new_value = 0, std::size_t new_pointer = 0)
    : value( new_value), pointer( new_pointer) {}

    int value;
    std::size_t pointer;
};

bool operator == (const node &lhs, const node &rhs) { return lhs.value == rhs.value; }
bool operator< (const node &lhs, const node &rhs) { return lhs.value < rhs.value; }

class Pseudo_LRUCacheInfo : public CacheReplacementInterface
{
    public:
        explicit Pseudo_LRUCacheInfo( std::size_t ways);
        void touch( std::size_t way) override;
        std::size_t update() override;
        std::size_t get_ways() const override { return ways; }

        void set_to_erase( std::size_t ) override;
        void allocate( std::size_t ) override;

    private:
        enum flags { Left = 0, Right = 1};

        core::tree<node> lru_tree;
        void construct_tree( core::tree<node>::iterator node_it, std::size_t max_depth);
        void construct_leaf_layer();
        std::size_t calculate_depth() const;
        std::size_t which_sibling( core::tree<node>::iterator node_it);
        static int reverse_flag( int flag);

        const std::size_t ways;
        int leaf_iterator = 0;
        int node_iterator = INT8_MIN; //make shure it is less than 0
};

Pseudo_LRUCacheInfo::Pseudo_LRUCacheInfo( std::size_t ways)
    : ways( ways)
{
    lru_tree.data( node(node_iterator));
    construct_tree( lru_tree.get_tree_iterator(), calculate_depth());
}

std::size_t Pseudo_LRUCacheInfo::calculate_depth() const
{
    std::size_t i = 0;
        for ( std::size_t numerator = ways; numerator != 1; numerator = numerator/2)
        {
            if (numerator%2 != 0)
                throw CacheReplacementException("Number of ways must be the power of 2!");
            i++;
        }
    return i;
}

void Pseudo_LRUCacheInfo::construct_tree( core::tree<node>::iterator node_it, std::size_t max_depth)
{
    if ( node_it.level() < max_depth)
    {
        if ( node_it.level() != max_depth - 1)
        {
            auto left_it = node_it.insert( node( node_iterator++, Left));
            auto right_it = node_it.insert( node( node_iterator++, Left));
            construct_tree( left_it, max_depth);
            construct_tree( right_it, max_depth);
        }
        else
        {
            node_it.insert( node(leaf_iterator++));
            node_it.insert( node(leaf_iterator++));
        }
    }
}

int Pseudo_LRUCacheInfo::reverse_flag( int flag) //more readable than multiplying on -1
{
    if ( flag == Left)
        return Right;
    else
        return Left;
}

std::size_t Pseudo_LRUCacheInfo::which_sibling( core::tree<node>::iterator node_it) //tree container doesnt provide such an option
{
    if ( node_it.data().value % 2 == 0)
        return Left;
    return Right;
}

void Pseudo_LRUCacheInfo::touch( std::size_t way)
{
    auto found_it = lru_tree.tree_find_depth( node( way));
    for ( auto i = found_it; i != lru_tree.get_tree_iterator(); i = i.out())
        if ( which_sibling( i) == i.out().data().pointer)
            i.out().data().pointer = reverse_flag(i.out().data().pointer);
}

std::size_t Pseudo_LRUCacheInfo::update()
{
    auto node_it = lru_tree.get_tree_iterator();
    while ( node_it.size() != 0)
    {
        if (node_it.data().pointer == Left)
            node_it = node_it.begin();
        else
            node_it = ++node_it.begin();
    }
    touch( node_it.data().value);
    return node_it.data().value;
}

void Pseudo_LRUCacheInfo::set_to_erase( std::size_t ) { throw CacheReplacementException( "Set_to_erase method is not supposed to be used in perfomance simulation"); }
void Pseudo_LRUCacheInfo::allocate( std::size_t ) { throw CacheReplacementException( "Allocate method is not supposed to be used in perfomance simulation"); }


////////////////////////////////////////////////////////////////////////////////////


std::unique_ptr<CacheReplacementInterface> create_cache_replacement( const std::string& name, std::size_t ways)
{
    if (name == "LRU")
        return std::make_unique<LRUCacheInfo>( ways);

    if (name == "Pseudo-LRU")
        return std::make_unique<Pseudo_LRUCacheInfo>( ways);

    throw CacheReplacementException("\"" + name + "\" replacement policy is not defined, supported polices are:\nLRU\npseudo-LRU\n");
}
