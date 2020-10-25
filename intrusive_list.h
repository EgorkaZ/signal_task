#pragma once

#include <iterator>
#include <iostream>
#include <type_traits>
#include <utility>

namespace  intrusive
{
/*
The default tag so that users don't need to
come up with tags if they only use one base
    list_element.
*/
struct  default_tag ;

template < typename  Tag = default_tag >
struct  list_element
{
    /* Unlinks an item from the list it is in. */
    list_element * next;
    list_element * prev;

    list_element() : next(this), prev(this) { }

    void link(list_element * prev, list_element * next)
    {
        this->next = next;
        this->prev = prev;

        next->prev = this;
        prev->next = this;
    }

    void  unlink ()
    {
        this->next->prev = prev;
        this->prev->next = next;

        this->prev = this->next = this;
    }

    ~list_element() {
        unlink();
    };
};


template < typename  T , typename  Tag = default_tag >
struct  list
{
private:
    template<bool is_const> class Iterator;

    static_assert ( std::is_convertible< T &, list_element < Tag > &>::value,
                    "value type is not convertible to list_element");

public:
    using iterator = Iterator<false>;
    using const_iterator = Iterator<true>;

    list () noexcept
    {
        m_fake_node.next = m_fake_node.prev = &m_fake_node;
    }
    list ( list  const &) = delete ;
    list ( list && other) noexcept
    {
        m_fake_node.next = m_fake_node.prev = &m_fake_node;
        operator=(std::move(other));
    }
    ~ list () {
    }

    list & operator = ( list  const &) = delete ;
    list & operator = ( list && other) noexcept
    {
        if (&other == this) {
            return *this;
        }
        if (other.empty()) {
            clear();
        } else {
            m_fake_node.link(other.m_fake_node.prev, other.m_fake_node.next);
            other.clear();
        }
        return *this;
    }

    void clear () noexcept { m_fake_node.next = m_fake_node.prev = &m_fake_node; }

    /*
    Since insertion changes the data in list_element
        we accept non-const T &.
    */
    void  push_back ( T & element) noexcept
    { static_cast<list_element<Tag> &>(element).link(m_fake_node.prev, &m_fake_node); }
    void  pop_back () noexcept { m_fake_node.prev->unlink(); }

    T & back () noexcept { return static_cast<T &>(*m_fake_node.prev); }
    T  const & back () const  noexcept { return static_cast<T const &>(*m_fake_node.prev); }

    void  push_front ( T & element) noexcept
    { static_cast<list_element<Tag> &>(element).link(&m_fake_node, m_fake_node.next); }
    void  pop_front () noexcept { m_fake_node.next->unlink(); }

    T & front () noexcept { return static_cast<T &>(*m_fake_node.next); }
    T  const & front () const  noexcept { return static_cast<T const &>(*m_fake_node.next); }

    bool  empty () const  noexcept { return m_fake_node.next == &m_fake_node && m_fake_node.prev == &m_fake_node; }

    iterator  begin () noexcept { return iterator(m_fake_node.next); }
    const_iterator  begin () const  noexcept { return const_iterator(m_fake_node.next); }

    iterator  end () noexcept { return iterator(&m_fake_node); }
    const_iterator  end () const  noexcept { return const_iterator(&m_fake_node); }

    iterator  insert ( const_iterator pos, T & element) noexcept
    {
        element.link(pos.m_element->prev, pos.m_element);
        return iterator(&element);
    }
    iterator  erase ( iterator pos) noexcept
    {
        pos++;
        pos->prev->unlink();
        return iterator(pos);
    }
    void  splice ( const_iterator pos, list &, const_iterator begin, const_iterator end) noexcept
    {
        if (begin == end) {
            return;
        }
        begin.m_element->prev->next = end.m_element;
        auto last = end.m_element->prev;
        end.m_element->prev = begin.m_element->prev;

        begin.m_element->prev = pos.m_element->prev;
        pos.m_element->prev->next = begin.m_element;

        last->next = pos.m_element;
        pos.m_element->prev = last;

    }

private:
    template <bool is_const>
    class Iterator {
        friend struct list<T, Tag>;

        template <class C>
        using add_constness = std::conditional_t<is_const, std::add_const_t<C>, std::remove_const_t<C>>;

        Iterator(list_element<Tag> * element) : m_element(element) { }
    public:
        using value_type = add_constness<T>;
        using pointer = add_constness<T> *;
        using reference = add_constness<T> &;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::bidirectional_iterator_tag;

        Iterator() = default;

        // enable if other's constness is not weaker then current
        template<bool is_other_const, std::enable_if_t<!is_other_const || is_const, int> = 0>
        Iterator(const Iterator<is_other_const> & other) : m_element(other.m_element) { }

        ~Iterator() = default;

        reference operator * () const noexcept { return static_cast<reference>(*m_element); }
        pointer operator -> () const noexcept { return static_cast<pointer>(m_element); }

        Iterator & operator ++ ()
        {
            m_element = m_element->next;
            return *this;
        }
        Iterator operator ++ (int)
        {
            auto res = *this;
            operator++();
            return res;
        }

        Iterator & operator -- ()
        {
            m_element = m_element->prev;
            return *this;
        }
        Iterator operator -- (int)
        {
            auto res = *this;
            operator--();
            return res;
        }

        bool operator == (const Iterator & rhs) const noexcept { return m_element == rhs.m_element; }
        bool operator != (const Iterator & rhs) const noexcept { return m_element != rhs.m_element; }

    private:
        list_element<Tag> * m_element;
    };

    mutable list_element<Tag> m_fake_node;
};
}
