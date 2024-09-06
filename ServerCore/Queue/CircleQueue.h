#pragma once
#include <ppl.h>
#include <concurrent_queue.h>

template <typename T>
class CircleQueue
{
public:
	CircleQueue(size_t size) : max_size(size) {}

	void enqueue(T item) {
		if (queue.size() < max_size) {
			queue.push(item);
		}
	}

	T dequeue() {
		T item;
		if (queue.try_pop(item)) {
			return item;
		}

		throw std::runtime_error("Queue is empty!!!");
	}

private:
	Concurrency::concurrent_queue<T> queue;
	size_t max_size;
};

