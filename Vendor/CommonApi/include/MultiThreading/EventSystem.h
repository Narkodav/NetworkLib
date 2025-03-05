#pragma once
#include "../Namespaces.h"
#include "Synchronized.h"

#include <functional>
#include <array>
#include <stdexcept>
#include <any>

namespace MultiThreading
{
	template<typename Policy>

	class EventSystem
	{
	public:
		using EventEnum = Policy::Enum;

		template<EventEnum E>
		using EventTraits = typename Policy::template Traits<E>;

		template<EventEnum E>
		using Callback = std::function<typename EventTraits<E>::Signature>;

		template<EventEnum E>
		using Subscribers = std::vector<Callback<E>>;

		using SunscriberMap = Synchronized<std::unordered_map<EventEnum, std::any>>;

	
		class Subscription
		{
			using EventEnum = Policy::Enum;
			EventSystem<Policy>* m_system;
			EventEnum m_event;
			std::function<void()> m_unsubscribe;
			bool m_active;

		public:
			Subscription(EventSystem<Policy>* system, EventEnum event, std::function<void()> unsubscribe)
				: m_system(system)
				, m_event(event)
				, m_unsubscribe(std::move(unsubscribe))
				, m_active(1)
			{}

			// Move constructor
			Subscription(Subscription&& other) noexcept
				: m_system(other.m_system)
				, m_event(other.m_event)
				, m_unsubscribe(std::move(other.m_unsubscribe))
				, m_active(other.m_active)
			{
				other.m_active = 0;
			}

			// Move assignment
			Subscription& operator=(Subscription&& other) noexcept {
				if (this != &other) {
					unsubscribe();
					m_system = other.m_system;
					m_event = other.m_event;
					m_unsubscribe = std::move(other.m_unsubscribe);
					m_active = other.m_active;
					other.m_active = false;
				}
				return *this;
			}

			// Disable copying
			Subscription(const Subscription&) = delete;
			Subscription& operator=(const Subscription&) = delete;

			// Explicit unsubscribe
			void unsubscribe() {
				if (m_active) {
					m_unsubscribe();
					m_active = 0;
				}
			}

			// Auto-unsubscribe on destruction
			~Subscription() {
				unsubscribe();
			}

			bool isActive() const { return m_active; }
		};

	private:

		SunscriberMap m_subscriberMap;

		// Helper function to create the correct subscriber vector for a given event
		template<EventEnum E>
		static std::any createSubscriberVector() {
			return std::any(Subscribers<E>{});
		}

		// Helper to build the array of creator functions
		template<size_t... Is>
		static constexpr auto makeSubscriberCreators(std::index_sequence<Is...>) {
			return std::array{ &createSubscriberVector<static_cast<EventEnum>(Is)>... };
		}

		// Array of functions to create subscribers, one for each event
		using SubscriberCreator = std::any(*)();
		static constexpr std::array<SubscriberCreator, static_cast<size_t>(EventEnum::EVENT_NUM)>
			createSubscriberFunctions = makeSubscriberCreators(std::make_index_sequence<static_cast<size_t>(EventEnum::EVENT_NUM)>{});

		template<EventEnum E>
		static bool compareCallbacks(const Callback<E>& a, const Callback<E>& b) {
			return a.target_type() == b.target_type() &&
				(a.target<typename EventTraits<E>::Signature*>() == b.target<typename EventTraits<E>::Signature*>());
		}

		template<EventEnum E>
		const Subscribers<E>& getSubscribers(SunscriberMap::ReadAccess& access) {
			auto it = access->find(E);
			if (it == access->end()) {
				throw std::runtime_error("Invalid event enum");
			}
			return std::any_cast<const Subscribers<E>&>(it->second);
		}

		template<EventEnum E>
		Subscribers<E>& getSubscribers(SunscriberMap::WriteAccess& access) {
			auto it = access->find(E);
			if (it == access->end()) {
				throw std::runtime_error("Invalid event enum");
			}
			return std::any_cast<Subscribers<E>&>(it->second);
		}

	public:

		EventSystem() {
			auto access = m_subscriberMap.getWriteAccess();
			for (int i = 0; i < static_cast<int>(EventEnum::EVENT_NUM); ++i) {
				access->insert({
					static_cast<EventEnum>(i),
					createSubscriberFunctions[i]()
					});
			}
		}

		template<EventEnum E>
		Subscription subscribe(Callback<E> callback) {
			if (!callback) {
				throw std::invalid_argument("Callback cannot be null");
			}

			auto access = m_subscriberMap.getWriteAccess();
			auto& subscribers = getSubscribers<E>(access);
			subscribers.push_back(callback);

			// Create unsubscribe function that captures the callback
			auto unsubscribe = [this, callback]() {
				auto access = m_subscriberMap.getWriteAccess();
				auto& subs = this->getSubscribers<E>(access);
				subs.erase(
					std::remove_if(
						subs.begin(),
						subs.end(),
						[this, &callback](const Callback<E>& cb) {
							return compareCallbacks<E>(cb, callback);
						}
					),
					subs.end()
				);
				};

			return Subscription(this, E, std::move(unsubscribe));
		}

		// Variadic template to handle different parameter counts
		template<EventEnum E, typename... Args>
		void emit(Args&&... args) {
			static_assert(std::is_invocable_v<typename EventSystem::EventTraits<E>::Signature, Args...>,
				"Parameter types don't match event signature");
			auto access = m_subscriberMap.getReadAccess();
			try {
				auto& subscribers = getSubscribers<E>(access);
				for (auto& subscriber : subscribers) {
					subscriber(std::forward<Args>(args)...);
				}
			}
			catch (const std::bad_any_cast& e) {
				throw std::runtime_error("Type mismatch in event emission: " + std::string(e.what()));
			}
		}

		template<EventEnum E>
		bool hasSubscribers() const {
			auto access = m_subscriberMap.getReadAccess();
			try {
				const auto& subscribers = getSubscribers<E>(access);
				return !subscribers.empty();
			}
			catch (const std::bad_any_cast&) {
				return false;
			}
		}

		template<EventEnum E>
		void clearSubscribers() {
			auto access = m_subscriberMap.getWriteAccess();
			auto& subscribers = getSubscribers<E>(access);
			subscribers.clear();
		}
	};
}