#pragma once

#include "intrusive_list.h"

#include <algorithm>
#include <functional>
#include <utility>

namespace signals {

template <typename T>
struct signal;

template <typename... Args>
struct signal<void(Args...)>
{
    struct connection;
    friend struct connection;

    using slot_t = std::function<void(Args...)>;

    struct connection : intrusive::list_element<struct connection_tag>
    {
        friend struct signal<void(Args...)>;

        connection() = default;

        explicit connection(const signal * sig, slot_t fun) noexcept;

        connection(const connection &) = delete;
        connection & operator=(const connection &) = delete;

        connection(connection && other) noexcept;
        connection & operator=(connection && other) noexcept;

        ~connection() noexcept;

        void disconnect() noexcept;

    private:
        void substitute_in_signal(connection & other) noexcept;

        const signal * m_sig = nullptr;
        slot_t m_fun = {};
    };

    using connections_t = intrusive::list<connection, struct connection_tag>;
    using conn_iterator = typename connections_t::iterator;

    struct iteration_token : intrusive::list_element<struct iteration_token_tag>
    {
        iteration_token(const signal * sig)
            : sig(sig)
            , it(sig->m_conns.begin())
        {
        }

        const signal * sig;
        conn_iterator it;
    };

    using iteration_tokens_t = intrusive::list<iteration_token, struct iteration_token_tag>;

    signal() = default;

    signal(signal const &) = delete;
    signal & operator=(signal const &) = delete;

    ~signal() noexcept;

    connection connect(slot_t slot) noexcept;

    void operator()(Args...) const;

private:
    mutable connections_t m_conns;
    mutable iteration_tokens_t m_iteration_tokens;
};

// signal implementation

template <typename... Args>
signal<void(Args...)>::~signal() noexcept
{
    for (auto it_conn = m_conns.begin(); it_conn != m_conns.end(); ) {
        auto it_curr = it_conn++;
        it_curr->disconnect();
    }
}

template <typename... Args>
typename signal<void(Args...)>::connection signal<void(Args...)>::connect(slot_t slot) noexcept
{
    return connection(this, std::move(slot));
}

template <typename... Args>
void signal<void(Args...)>::operator()(Args... args) const
{
    iteration_token itok(this);
    m_iteration_tokens.push_back(itok);

    auto conns_end = m_conns.end();

    for (auto & conn_it = itok.it; conn_it != conns_end; ++conn_it) {
        if (conn_it->m_fun) {
            conn_it->m_fun(args...);
            if (conn_it == conns_end) {
                return;
            }
        }
    }

    m_iteration_tokens.pop_back();
}

// signal::connection implementation
template <typename... Args>
signal<void(Args...)>::connection::connection(const signal * sig, slot_t fun) noexcept
    : m_sig(sig)
    , m_fun(std::move(fun))
{
    if (m_sig) {
        m_sig->m_conns.push_back(*this);
    }
}

template <typename... Args>
signal<void(Args...)>::connection::connection(connection && other) noexcept
    : m_sig(other.m_sig)
    , m_fun(std::move(other.m_fun))
{
    substitute_in_signal(other);
}

template <typename... Args>
typename signal<void(Args...)>::connection & signal<void(Args...)>::connection::operator=(connection && other) noexcept
{
    if (&other == this) {
        return *this;
    }
    disconnect();
    m_sig = other.m_sig;
    m_fun = std::move(other.m_fun);
    substitute_in_signal(other);
    return *this;
}

template <typename... Args>
signal<void(Args...)>::connection::~connection() noexcept
{
    disconnect();
}

template <typename... Args>
void signal<void(Args...)>::connection::disconnect() noexcept
{
    if (!m_sig) {
        return;
    }
    auto & it_toks = m_sig->m_iteration_tokens;
    std::for_each(it_toks.begin(), it_toks.end(), [this](iteration_token & it_tok) {
        if (&(*it_tok.it) == this) {
            --it_tok.it;
        }
    });

    auto & conns = m_sig->m_conns;
    unlink();
    m_sig = nullptr;
}

template <typename... Args>
void signal<void(Args...)>::connection::substitute_in_signal(connection & other) noexcept
{
    if (m_sig) {
        m_sig->m_conns.insert(m_sig->m_conns.as_iterator(other), *this);
        other.disconnect();
    }
    other.m_sig = nullptr;
}

} // namespace signals
