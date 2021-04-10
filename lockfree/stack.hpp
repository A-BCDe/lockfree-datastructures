#ifndef	LOCKFREE_STACK_HPP
#define	LOCKFREE_STACK_HPP

#include <atomic>
#include <cstdio>
#include <memory>

namespace lockfree {
	template<typename T>
	class stack {
		class node {
		public:
			node() : data() {}
			node(T* data, node* next) : data(std::move(data)) {
				this->next.store(next, std::memory_order_release);
			}
			~node() {}
			T* data;
			std::atomic<node*> next;
		};
	public:
		stack() {
			tail.store(new node(nullptr, nullptr), std::memory_order_release);
		}

		~stack() { // should happen when no process/thread is accessing.
			while (pop());
		}

		[[deprecated("Unstable")]] bool empty() const {
			return tail.load(std::memory_order_relaxed) == nullptr;
		}

		[[deprecated("Unstable")]] T const* top() const {
			return tail.load(std::memory_order_relaxed)->data;
		}

		void push(T data) {
			// tail -> node1 -> node2
			//			 ^
			//  n       -+
			auto n = new node(new T(std::move(data)), nullptr);
			auto tail_cur = tail.load(std::memory_order_relaxed);
			
			do {
				n->next.store(tail_cur, std::memory_order_release);
			} while (!tail.compare_exchange_weak(tail_cur, n, std::memory_order_release, std::memory_order_relaxed));
		}

		std::unique_ptr<T> pop() {
			// tail -> node1 -> node2
			node* node1;
			node* tail_cur = tail.load(std::memory_order_relaxed);
			T* value;
			do {
				if (!tail_cur->next.load(std::memory_order_relaxed)) return nullptr;
				node1 = tail_cur->next.load(std::memory_order_acquire);
				value = tail_cur->data;
			} while (!tail.compare_exchange_weak(tail_cur, node1, std::memory_order_acquire, std::memory_order_relaxed));
			delete tail_cur;
			return std::unique_ptr<T>(value);
		}

	private:
		std::atomic<node*> tail;
	};
}

#endif