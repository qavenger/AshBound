#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

struct DelegateHandle
{
	static constexpr std::uint64_t InvalidId = 0;

	std::uint64_t id = InvalidId;

	bool IsValid() const { return id != InvalidId; }

	static DelegateHandle Generate()
	{
		static std::atomic<std::uint64_t> s_nextId{ 1 };
		DelegateHandle handle;
		handle.id = s_nextId.fetch_add(1, std::memory_order_relaxed);
		return handle;
	}

	static constexpr DelegateHandle Invalid()
	{
		return DelegateHandle{ InvalidId };
	}

	bool operator==(const DelegateHandle& other) const { return id == other.id; }
	bool operator!=(const DelegateHandle& other) const { return id != other.id; }
};

template<typename Signature>
class Delegate;

template<typename R, typename... Args>
class Delegate<R(Args...)>
{
public:
	Delegate() = default;

	template<typename Callable, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Callable>, Delegate>>>
	Delegate(Callable&& callable)
	{
		Bind(std::forward<Callable>(callable));
	}

	template<typename Callable>
	static Delegate Create(Callable&& callable)
	{
		Delegate delegate;
		delegate.Bind(std::forward<Callable>(callable));
		return delegate;
	}

	template<typename T>
	static Delegate Create(T* instance, R(T::*method)(Args...))
	{
		return Delegate([instance, method](Args... args)
		{
			return (instance->*method)(std::forward<Args>(args)...);
		});
	}

	template<typename T>
	static Delegate Create(const T* instance, R(T::*method)(Args...) const)
	{
		return Delegate([instance, method](Args... args)
		{
			return (instance->*method)(std::forward<Args>(args)...);
		});
	}

	template<typename Callable>
	void Bind(Callable&& callable)
	{
		using Decayed = std::decay_t<Callable>;
		m_instance = std::make_shared<DelegateInstance<Decayed>>(std::forward<Callable>(callable));
		m_handle = DelegateHandle::Generate();
	}

	void Unbind()
	{
		m_instance.reset();
		m_handle = DelegateHandle::Invalid();
	}

	bool IsBound() const { return m_instance != nullptr; }
	explicit operator bool() const { return IsBound(); }

	const DelegateHandle& GetHandle() const { return m_handle; }

	R Execute(Args... args) const
	{
		if (!m_instance)
		{
			if constexpr (!std::is_void_v<R>)
			{
				return R();
			}
			else
			{
				return;
			}
		}
		return m_instance->Execute(std::forward<Args>(args)...);
	}

	R operator()(Args... args) const
	{
		return Execute(std::forward<Args>(args)...);
	}

	bool operator==(const Delegate& other) const { return m_handle == other.m_handle; }
	bool operator!=(const Delegate& other) const { return m_handle != other.m_handle; }

private:
	struct IDelegateInstance
	{
		virtual ~IDelegateInstance() = default;
		virtual R Execute(Args... args) = 0;
	};

	template<typename Callable>
	struct DelegateInstance final : IDelegateInstance
	{
		explicit DelegateInstance(Callable&& callable)
			: m_callable(std::forward<Callable>(callable))
		{
		}

		R Execute(Args... args) override
		{
			return std::invoke(m_callable, std::forward<Args>(args)...);
		}

		Callable m_callable;
	};

	DelegateHandle m_handle = DelegateHandle::Invalid();
	std::shared_ptr<IDelegateInstance> m_instance;
};
