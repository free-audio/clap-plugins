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

      T *next = nullptr;
      T *prev = nullptr;
   };

   template <typename T, IntrusiveListHook<T> T::*Member>
   class IntrusiveList;

   template <typename T, IntrusiveListHook<T> T::*Member>
   class IntrusiveListIterator {
   public:
      inline IntrusiveListIterator(T *item) : _item(item) {}

      inline T &operator*() const { return *_item; }
      inline T *operator->() const { return _item; }
      inline IntrusiveListIterator<T, Member> &operator++() {
         _item = (_item->*Member).next_;
         return *this;
      }

      inline IntrusiveListIterator<T, Member> &operator--() {
         _item = (_item->*Member).prev_;
         return *this;
      }

      inline bool operator==(const IntrusiveListIterator<T, Member> &other) const {
         return _item == other._item;
      }

      inline bool operator!=(const IntrusiveListIterator<T, Member> &other) const {
         return !(*this == other);
      }

   private:
      T *_item = nullptr;
   };

   template <typename T, IntrusiveListHook<T> T::*Member>
   class IntrusiveListReverseIterator {
   public:
      inline IntrusiveListReverseIterator(T *item) : _item(item) {}

      inline T &operator*() const { return *_item; }
      inline T *operator->() const { return _item; }
      inline IntrusiveListReverseIterator<T, Member> &operator--() {
         _item = (_item->*Member).next_;
         return *this;
      }

      inline IntrusiveListReverseIterator<T, Member> &operator++() {
         _item = (_item->*Member).prev_;
         return *this;
      }

      inline bool operator==(const IntrusiveListReverseIterator<T, Member> &other) const {
         return _item == other._item;
      }

      inline bool operator!=(const IntrusiveListReverseIterator<T, Member> &other) const {
         return !(*this == other);
      }

   private:
      T *_item;
   };

   template <typename T, IntrusiveListHook<T> T::*Member>
   class IntrusiveList {
   public:
      typedef IntrusiveListIterator<T, Member> iterator;
      typedef IntrusiveListReverseIterator<T, Member> reverse_iterator;

      inline IntrusiveList() : head_(nullptr), tail_(nullptr), size_(0) {}
      inline ~IntrusiveList() {
         while (!empty())
            popBack();
      }

      inline bool empty() const { return !head_; }

      inline size_t size() const { return size_; }

      inline void pushFront(T *item) {
         ++size_;

         if (head_)
            (head_->*Member).prev_ = item;
         else
            tail_ = item;
         (item->*Member).next_ = head_;
         (item->*Member).prev_ = nullptr;
         head_ = item;
      }

      inline void pushBack(T *item) {
         ++size_;

         if (tail_)
            (tail_->*Member).next_ = item;
         else
            head_ = item;
         (item->*Member).prev_ = tail_;
         (item->*Member).next_ = nullptr;
         tail_ = item;
      }

      inline T *front() const {
         assert(head_);
         return head_;
      }

      inline T *back() const {
         assert(tail_);
         return tail_;
      }

      inline void popFront() {
         if (!head_)
            return;

         --size_;

         if (head_ == tail_) {
            head_ = nullptr;
            tail_ = nullptr;
            return;
         }

         head_ = (head_->*Member).next_;
         (head_->*Member).prev_ = nullptr;
      }

      inline void popBack() {
         if (!tail_)
            return;

         --size_;

         if (head_ == tail_) {
            head_ = nullptr;
            tail_ = nullptr;
            return;
         }

         tail_ = (tail_->*Member).prev_;
         (tail_->*Member).next_ = nullptr;
      }

      inline void clear() {
         // poping everything to unlink (and potentially release while unref)
         while (!empty())
            popBack();
      }

      inline void erase(T *item) {
         if ((item->*Member).prev_)
            ((item->*Member).prev_->*Member).next_ = (item->*Member).next_;
         else
            head_ = (item->*Member).next_;

         if ((item->*Member).next_)
            ((item->*Member).next_->*Member).prev_ = (item->*Member).prev_;
         else
            tail_ = (item->*Member).prev_;
      }

      iterator begin() const { return empty() ? end() : iterator(head_); }
      iterator end() const { return iterator(nullptr); }

      reverse_iterator rbegin() const { return empty() ? rend() : reverse_iterator(tail_); }
      reverse_iterator rend() const { return reverse_iterator(nullptr); }

   private:
      T *head_ = nullptr;
      T *tail_ = nullptr;
      size_t size_ = 0;
   };
} // namespace clap
