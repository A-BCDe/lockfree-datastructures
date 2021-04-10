#ifndef LOCKFREE_QUEUE_HPP
#define	LOCKFREE_QUEUE_HPP

#include <atomic>
#include <memory>

namespace lockfree {
	template<typename T>
	class queue {
		class node {
		public:
			node() : data() {}
			node(T* data, node* next) : data(data) {
				this->next.store(next);
			}
			~node() {}
			T* data;
			std::atomic<node*> next;
		};

	public:
		queue() {
			node* dummy = new node(nullptr, nullptr);
			head.store(dummy);
			tail.store(dummy);
		}

		~queue() { // should happen when no process/thread is accessing.
			while (pop());
			node* dummy = head.load(std::memory_order_acquire);
			delete dummy;
		}

		[[deprecated("Unstable")]] bool empty() const {
			return head.load(std::memory_order_relaxed) == tail.load(std::memory_order_relaxed);
		}

		[[deprecated("Unstable")]] T* const& front() const {
			return head.load(std::memory_order_relaxed)->data;
		}

		void push(T data) {
			//                tail    n -+
			//                 |      ^  |
			//				   v   +--+  v
			// ... -> node2 -> node1 -> null

			node* n = new node(new T(std::move(data)), nullptr);
			node* tail_cur;
			node* next;

			while (true) {
				tail_cur = tail.load(std::memory_order_relaxed);
				next = tail_cur->next.load(std::memory_order_relaxed);
				if (tail_cur == tail.load(std::memory_order_acquire)) {
					if (next == nullptr) {
						if (tail_cur->next.compare_exchange_weak(next, n, std::memory_order_release, std::memory_order_relaxed)) break;
					}
					else {
						tail.compare_exchange_weak(tail_cur, next, std::memory_order_release, std::memory_order_relaxed);
					}
				}
			}
			tail.compare_exchange_weak(tail_cur, n, std::memory_order_release, std::memory_order_relaxed);
		}

		std::unique_ptr<T> pop() {
			// head
			//	|  +-------------+
			//  v  |             v
			// dummy -> node1 -> node2 -> ...
			node* head_cur;
			node* tail_cur;
			node* next;
			T* value;
			while (true) {
				head_cur = head.load(std::memory_order_relaxed);
				tail_cur = tail.load(std::memory_order_relaxed);
				next = head_cur->next.load(std::memory_order_relaxed);
				if (head_cur == head.load(std::memory_order_acquire)) {
					if (head_cur == tail_cur) {
						if (next == nullptr) return nullptr;
						tail.compare_exchange_weak(tail_cur, next, std::memory_order_acquire, std::memory_order_relaxed);
					}
					else {
						value = next->data;
						if (head.compare_exchange_weak(head_cur, next, std::memory_order_acquire, std::memory_order_relaxed)) break;
					}
				}
			}
			delete head_cur;
			return std::make_unique<T>(std::forward<T>(*value));
		}
	private:
		std::atomic<node*> head, tail;
	};
}

#endif