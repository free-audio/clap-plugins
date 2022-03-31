#pragma once

#include <cassert>
#include <cstddef>

namespace clap {
   template <typename T>
   class IntrusiveListHook {
   public:
      IntrusiveListHook() = default;
      IntrusiveListHook(const IntrusiveListHook<T> &) {}
      IntrusiveListHook(IntrusiveListHook<T> &&) {}
      IntrusiveListHook<T> &operator=(const IntrusiveListHook<T> &) { return *this; }
      IntrusiveListHook<T> &operator=(IntrusiveListHook<T> &&) { return *this; }

      [[nodiscard]] bool isHooked() const noexcept { return _next == nullptr && _prev == nullptr; }

      T *_next = nullptr;
      T *_prev = nullptr;
   };

   template <typename T, IntrusiveListHook<T> T::*Member>
   class IntrusiveList;

   template <typename T, IntrusiveListHook<T> T::*Member>
   class IntrusiveListIterator {
   public:
      inline IntrusiveListIterator(T *item) : _item(item) {}

      [[nodiscard]] inline T &operator*() const { return *_item; }
      [[nodiscard]] inline T *operator->() const { return _item; }
      inline IntrusiveListIterator<T, Member> &operator++() {
         _item = (_item->*Member)._next;
         return *this;
      }

      inline IntrusiveListIterator<T, Member> &operator--() {
         _item = (_item->*Member)._prev;
         return *this;
      }

      [[nodiscard]] inline bool operator==(const IntrusiveListIterator<T, Member> &other) const {
         return _item == other._item;
      }

      [[nodiscard]] inline bool operator!=(const IntrusiveListIterator<T, Member> &other) const {
         return !(*this == other);
      }

   private:
      T *_item = nullptr;
   };

   template <typename T, IntrusiveListHook<T> T::*Member>
   class IntrusiveListReverseIterator {
   public:
      inline IntrusiveListReverseIterator(T *item) : _item(item) {}

      [[nodiscard]] inline T &operator*() const { return *_item; }
      [[nodiscard]] inline T *operator->() const { return _item; }
      inline IntrusiveListReverseIterator<T, Member> &operator--() {
         _item = (_item->*Member)._next;
         return *this;
      }

      inline IntrusiveListReverseIterator<T, Member> &operator++() {
         _item = (_item->*Member)._prev;
         return *this;
      }

      [[nodiscard]] inline bool
      operator==(const IntrusiveListReverseIterator<T, Member> &other) const {
         return _item == other._item;
      }

      [[nodiscard]] inline bool
      operator!=(const IntrusiveListReverseIterator<T, Member> &other) const {
         return !(*this == other);
      }

   private:
      T *_item = nullptr;
   };

   template <typename T, IntrusiveListHook<T> T::*Member>
   class IntrusiveList {
   public:
      typedef IntrusiveListIterator<T, Member> iterator;
      typedef IntrusiveListReverseIterator<T, Member> reverse_iterator;

      inline IntrusiveList() : _head(nullptr), _tail(nullptr), _size(0) {}
      inline ~IntrusiveList() {
         while (!empty())
            popBack();
      }

      [[nodiscard]] inline bool empty() const { return !_head; }

      [[nodiscard]] inline size_t size() const { return _size; }

      inline void pushFront(T *item) {
         ++_size;

         if (_head)
            (_head->*Member)._prev = item;
         else
            _tail = item;
         (item->*Member)._next = _head;
         (item->*Member)._prev = nullptr;
         _head = item;
      }

      inline void pushBack(T *item) {
         ++_size;

         if (_tail)
            (_tail->*Member)._next = item;
         else
            _head = item;
         (item->*Member)._prev = _tail;
         (item->*Member)._next = nullptr;
         _tail = item;
      }

      inline T *front() const {
         return _head;
      }

      inline T *back() const {
         return _tail;
      }

      inline void popFront() {
         if (!_head)
            return;

         --_size;

         if (_head == _tail) {
            _head = nullptr;
            _tail = nullptr;
            return;
         }

         _head = (_head->*Member)._next;
         (_head->*Member)._prev = nullptr;
      }

      inline void popBack() {
         if (!_tail)
            return;

         --_size;

         if (_head == _tail) {
            _head = nullptr;
            _tail = nullptr;
            return;
         }

         _tail = (_tail->*Member)._prev;
         (_tail->*Member)._next = nullptr;
      }

      inline void clear() {
         // poping everything to unlink (and potentially release while unref)
         while (!empty())
            popBack();
      }

      inline void erase(T *item) {
         if ((item->*Member)._prev)
            ((item->*Member)._prev->*Member)._next = (item->*Member)._next;
         else
            _head = (item->*Member)._next;

         if ((item->*Member)._next)
            ((item->*Member)._next->*Member)._prev = (item->*Member)._prev;
         else
            _tail = (item->*Member)._prev;
      }

      iterator begin() const { return empty() ? end() : iterator(_head); }
      iterator end() const { return iterator(nullptr); }

      reverse_iterator rbegin() const { return empty() ? rend() : reverse_iterator(_tail); }
      reverse_iterator rend() const { return reverse_iterator(nullptr); }

   private:
      T *_head = nullptr;
      T *_tail = nullptr;
      size_t _size = 0;
   };
} // namespace clap
