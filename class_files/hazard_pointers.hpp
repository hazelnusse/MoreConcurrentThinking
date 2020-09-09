#ifndef HAZARD_POINTERS_HPP
#define HAZARD_POINTERS_HPP
#include <mutex>
#include <vector>
#include <algorithm>
#include <memory>
#include <atomic>

class hazard_pointer_domain;
class hazard_pointer;

hazard_pointer_domain &hazard_pointer_default_domain();

hazard_pointer make_hazard_pointer(
  hazard_pointer_domain &domain =
    hazard_pointer_default_domain());
void hazard_pointer_clean_up(
  hazard_pointer_domain &domain =
    hazard_pointer_default_domain());

template <typename T, typename D = std::default_delete<T>>
class hazard_pointer_obj_base;

class hazard_pointer_domain {
public:
  struct hp_base {
    virtual void do_cleanup() = 0;
    virtual ~hp_base() {}

  protected:
    void register_retiree(hazard_pointer_domain &domain);
  };

  hazard_pointer_domain(
    const hazard_pointer_domain &) = delete;
  hazard_pointer_domain(hazard_pointer_domain &&) = delete;
  hazard_pointer_domain &operator=(
    const hazard_pointer_domain &) = delete;
  hazard_pointer_domain &operator=(
    hazard_pointer_domain &&) = delete;

  hazard_pointer_domain() = default;
  ~hazard_pointer_domain();

private:
  friend class hazard_pointer;
  friend hazard_pointer make_hazard_pointer(
    hazard_pointer_domain &domain);
  friend void hazard_pointer_clean_up(
    hazard_pointer_domain &domain);

  hazard_pointer make_hazard_pointer();
  void free_hazard_pointer(size_t index);
  void cleanup();
  void internal_cleanup();
  void register_retiree(hp_base *ptr);

  bool is_protected(hp_base *ptr) const noexcept;

  std::mutex m;
  std::vector<size_t> free_pointers;
  std::vector<hp_base *> pointers;
  std::vector<hp_base *> pointers_to_reclaim;
};

inline hazard_pointer_domain &
hazard_pointer_default_domain() {
  static hazard_pointer_domain domain;
  return domain;
}

class hazard_pointer {
public:
  hazard_pointer() noexcept : domain(nullptr), index(0) {}

  hazard_pointer(hazard_pointer &&other) noexcept :
    domain(other.domain), index(other.index) {
    other.domain = nullptr;
    other.index = 0;
  }

  hazard_pointer &operator=(hazard_pointer &&other) noexcept {
    hazard_pointer temp(std::move(other));
    swap(temp);
    return *this;
  }

  hazard_pointer(const hazard_pointer &) = delete;
  hazard_pointer &operator=(const hazard_pointer &) = delete;
  ~hazard_pointer() {
    if(domain) {
      domain->free_hazard_pointer(index);
    }
  }

  bool empty() const noexcept {
    return !domain;
  }

  template <typename T>
  T *protect(const std::atomic<T *> &src) noexcept {
    std::lock_guard guard(domain->m);
    auto &ptr = domain->pointers[index];
    T *res;
    do {
      res = src.load();
      ptr = res;
    } while(src.load() != ptr);
    return res;
  }

  template <typename T>
  bool try_protect(
    T *&ptr, const std::atomic<T *> &src) noexcept {
    std::lock_guard guard(domain->m);
    auto &protected_ptr = domain->pointers[index];
    protected_ptr = src.load();
    if((protected_ptr == ptr) &&
      (src.load() == protected_ptr)) {
      return true;
    } else {
      protected_ptr = nullptr;
      return false;
    }
  }

  template <typename T>
  void reset_protection(const T *ptr) noexcept {
    std::lock_guard guard(domain->m);
    domain->pointers[index] = ptr;
  }

  void reset_protection(nullptr_t = nullptr) noexcept {
    std::lock_guard guard(domain->m);
    domain->pointers[index] = nullptr;
  }

  void swap(hazard_pointer &other) noexcept {
    std::swap(domain, other.domain);
    std::swap(index, other.index);
  }

private:
  friend class hazard_pointer_domain;

  hazard_pointer(
    hazard_pointer_domain *domain_, unsigned index_) :
    domain(domain_),
    index(index_) {}

  hazard_pointer_domain *domain;
  size_t index;
};

inline hazard_pointer make_hazard_pointer(
  hazard_pointer_domain &domain) {
  return domain.make_hazard_pointer();
}

inline void hazard_pointer_domain::free_hazard_pointer(
  size_t index) {
  std::lock_guard guard(m);
  free_pointers.push_back(index);
}

inline hazard_pointer hazard_pointer_domain::
  make_hazard_pointer() {
  std::lock_guard guard(m);
  if(!free_pointers.empty()) {
    auto index = free_pointers.back();
    free_pointers.resize(free_pointers.size() - 1);
    return hazard_pointer(this, index);
  }
  pointers.push_back(nullptr);
  return hazard_pointer(this, pointers.size() - 1);
}

inline void hazard_pointer_clean_up(
  hazard_pointer_domain &domain) {
  domain.cleanup();
}

inline hazard_pointer_domain::~hazard_pointer_domain() {
  cleanup();
}

inline bool hazard_pointer_domain::is_protected(
  hp_base *ptr) const noexcept {
  for(auto &protected_ptr : pointers) {
    if(ptr == protected_ptr) {
      return true;
    }
  }
  return false;
}

inline void hazard_pointer_domain::cleanup() {
  std::lock_guard guard(m);
  internal_cleanup();
}
inline void hazard_pointer_domain::internal_cleanup() {
  std::vector<hp_base *> remainder;
  for(auto &ptr : pointers_to_reclaim) {
    if(is_protected(ptr)) {
      remainder.push_back(ptr);
    } else {
      ptr->do_cleanup();
    }
  }
  pointers_to_reclaim.swap(remainder);
}

inline void hazard_pointer_domain::hp_base::register_retiree(
  hazard_pointer_domain &domain) {
  domain.register_retiree(this);
}

inline void hazard_pointer_domain::register_retiree(
  hp_base *ptr) {
  std::lock_guard guard(m);
  pointers_to_reclaim.push_back(ptr);
  if(pointers_to_reclaim.size() >= (pointers.size() * 2)) {
    internal_cleanup();
  }
}

template <typename T, typename D>
class hazard_pointer_obj_base
  : public hazard_pointer_domain::hp_base {
public:
  void retire(D reclaim = {},
    hazard_pointer_domain &domain =
      hazard_pointer_default_domain()) {
    deleter = reclaim;
    register_retiree(domain);
  }

  void retire(hazard_pointer_domain &domain) {
    retire({}, domain);
  }

protected:
  hazard_pointer_obj_base() = default;

private:
  void do_cleanup() {
    (*deleter)(static_cast<T *>(this));
  }

  std::optional<D> deleter;
};

#endif
