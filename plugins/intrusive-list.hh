#pragma once

#include <cassert>
#include <cstddef>

namespace clap {

   class IntrusiveList final {
   public:
      class Iterator;

      class Hook final {
         friend class Iterator;
         friend class IntrusiveList;

      public:
         constexpr Hook() = default;
         constexpr Hook(const Hook &) {}
         constexpr Hook(Hook &&) {}
         constexpr Hook &operator=(const Hook &) { return *this; }
         constexpr Hook &operator=(Hook &&) { return *this; }
         ~Hook()
         {
            assert(!isHooked());
         }

         [[nodiscard]] inline bool isHooked() const noexcept {
            assert(checkInvariants());
            return _next != this;
         }

         inline void unlink() noexcept {
            assert(checkInvariants());

            _next->_prev = _prev;
            _prev->_next = _next;
         }

         [[nodiscard]] inline bool checkInvariants() const noexcept {
            assert(_next == this ? _prev == this : true);
            assert(_prev == this ? _next == this : true);
            return true;
         }

      private:
         Hook *_next = this;
         Hook *_prev = this;
      };

      class Iterator final {
      public:
         inline Iterator(Hook *head, Hook *item) : _head(head), _item(item) {}
         inline Iterator(const Iterator& it) : _head(it._head), _item(it._item) {}

         [[nodiscard]] inline Hook *item() const {
            assert(!end());
            return _item;
         }

         inline Iterator &operator++() {
            _item = _item->_next;
            return *this;
         }

         inline Iterator &operator--() {
            _item = _item->_prev;
            return *this;
         }

         [[nodiscard]] inline bool operator==(const Iterator &other) const {
            assert(_head == other._head);
            return _item == other._item;
         }

         [[nodiscard]] inline bool operator!=(const Iterator &other) const {
            assert(_head == other._head);
            return _item != other._item;
         }

         [[nodiscard]] inline bool end() const noexcept { return _head == _item; }

      private:
         Hook *const _head;
         Hook *_item = nullptr;
      };

      inline IntrusiveList() = default;
      inline ~IntrusiveList() {
         clear();
      }

      [[nodiscard]] inline bool empty() const { return !_head.isHooked(); }

      inline void pushFront(Hook *item) {
         assert(!item->isHooked());

         if (!_head.isHooked()) {
            item->_next = &_head;
            item->_prev = &_head;
            _head._next = item;
            _head._prev = item;
         } else {
            item->_next = _head._next;
            item->_prev = &_head;
            _head._next->_prev = item;
            _head._next = item;
         }
      }

      inline void pushBack(Hook *item) {
         assert(!item->isHooked());

         if (!_head.isHooked()) {
            item->_next = &_head;
            item->_prev = &_head;
            _head._next = item;
            _head._prev = item;
         } else {
            item->_next = &_head;
            item->_prev = _head._prev;
            _head._prev->_next = item;
            _head._prev = item;
         }
      }

      inline Hook *front() const {
         assert(!empty());
         return _head._next;
      }

      inline Hook *back() const {
         assert(!empty());
         return _head._prev;
      }

      inline void popFront() {
         assert(!empty());
         _head._next->unlink();
      }

      inline void popBack() {
         assert(!empty());
         _head._prev->unlink();
      }

      inline void clear() {
         while (!empty())
            popBack();
      }

      Iterator begin() { return empty() ? end() : Iterator(&_head, _head._next); }
      Iterator end() { return {&_head, &_head}; }

   private:
      Hook _head;
   };
} // namespace clap
