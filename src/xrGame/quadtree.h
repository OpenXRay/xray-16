////////////////////////////////////////////////////////////////////////////
//	Module 		: quadtree.h
//	Created 	: 23.03.2004
//  Modified 	: 23.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Quadtree class
////////////////////////////////////////////////////////////////////////////

#pragma once
#include "xrEngine/profiler.h"

template <typename _object_type>
class CQuadTree
{
public:
    struct CQuadNode
    {
        CQuadNode* m_neighbours[4];

        IC CQuadNode*& next() { return (m_neighbours[0]); }
    };

    struct CListItem
    {
        _object_type* m_object;
        CListItem* m_next;

        IC CListItem*& next() { return (m_next); }
    };

    template <typename T>
    struct CStorage
    {
        T* m_free{};
        u32 m_max_object_count;
        xr_vector<std::unique_ptr<T, void(*)(T*)>> m_blocks;

        explicit CStorage(u32 max_object_count) : m_max_object_count(max_object_count)
        {
            create_block();
        }

        void create_block()
        {
            auto& block = m_blocks.emplace_back(xr_alloc<T>(m_max_object_count), [](T* p) { xr_free(p); });
            T* b = nullptr;
            T* i = block.get();
            T* e = i + m_max_object_count;
            for (; i != e; b = i, ++i)
                i->next() = b;
            m_free = e - 1;
        }

        T* get_object()
        {
            if (!m_free)
                create_block();
            T* node = m_free;
            m_free = m_free->next();
            ZeroMemory(node, sizeof(T));
            return node;
        }

        void clear()
        {
            m_blocks.clear();
        }

        void remove(T*& node)
        {
            node->next() = m_free;
            m_free = node;
            node = 0;
        }
    };

    using CQuadNodeStorage = CStorage<CQuadNode>;
    using CListItemStorage = CStorage<CListItem>;

protected:
    Fvector m_center;
    float m_radius;
    int m_max_depth;
    CQuadNode* m_root;
    CQuadNodeStorage* m_nodes;
    CListItemStorage* m_list_items;
    size_t m_leaf_count;

protected:
    IC u32 neighbour_index(const Fvector& position, Fvector& center, float distance) const;
    IC void nearest(const Fvector& position, float radius, xr_vector<_object_type*>& objects, CQuadNode* node,
        Fvector center, float distance, int depth) const;
    IC _object_type* remove(const _object_type* object, CQuadNode*& node, Fvector center, float distance, int depth);
    IC void all(xr_vector<_object_type*>& objects, CQuadNode* node, int depth) const;

public:
    IC CQuadTree(const Fbox& box, float min_cell_size, u32 max_node_count, u32 max_list_item_count);
    virtual ~CQuadTree();
    IC void clear();
    IC void insert(_object_type* object);
    IC _object_type* remove(const _object_type* object);
    IC _object_type* find(const Fvector& position) const;
    IC void nearest(const Fvector& position, float radius, xr_vector<_object_type*>& objects, bool clear = true) const;
    IC void all(xr_vector<_object_type*>& objects, bool clear = true) const;
    IC size_t size() const;
    IC bool empty() const;
};

#include "quadtree_inline.h"
