/*! \file       LTK_Events.h
**  \author     Luke Cardell
**  \date       10/18/2018
**  \brief
**    Events and event system designed to be
**    modular and reusable.
**
**  \see LTK_ScratchPad.h
**
**  \copyright  GNU Public License.
*/
#ifndef LTK_EVENTS_H
#define LTK_EVENTS_H

#include "LTK_ScratchPad.h"
#include <unordered_map>
#include <vector>
#include <set>
#include <algorithm>
#include <cstring>

#define LTK_EVENTS_PADSIZE 1024

//! Luke's Tool Kit
namespace LTK {
  /*! EventSys
  **  \brief
  **    Simple, portable event system.
  **  \par About:
  **    This event system is designed to be
  **    modular, as it is merely a system to use
  **    with custom-made events. It includes a
  **    basic event class hierarchy designed to be
  **    inherited from by user-defined events.
  **  \par Usage:
  **    This is a brief overview of how to use the
  **    event system.
  **    +#  Define a custom event class. For more
  **        information, refer to the Event class
  **        documentation.
  **    +#  A callback function should be defined.
  **        Callback functions can be static
  **        functions or member functions.
  **        For static functions, the function
  **        signature should be:
  **        + `bool (*)(IEvent *)`
  **        .
  **        The IEvent pointer should be replaced
  **        with a pointer to the user-defined
  **        event, but for consistency's sake this
  **        documentation will use the interface
  **        type as a placeholder.
  **        For member functions, the function
  **        signature should be:
  **        + `bool (Class::*)(IEvent *)`
  **        .
  **        Generally, callback functions should
  **        return false. The boolean return value
  **        is only used by the system for early
  **        cancellations of the event resolution
  **        loop.
  **    +#  The callback function should be
  **        subscribed to the event for which it
  **        is listening.
  **        This is done using the `Subscribe`
  **        function template. An example class
  **        subscription follows:
  **        + `es.Subscribe<Class, IEvent,
  **             &Class::Function>(this);`
  **        .
  **        An example static subscription
  **        + `es.Subscribe<IEvent, &Function>();`
  **        .
  **    +#  An event should be instantiated and
  **        activated using the `Activate`
  **        function template.
  **        During the activation process, the
  **        event will be. copied using the
  **        `Clone` function. For more
  **        information, refer to the
  **        documentation for `Event::Clone`.
  **    +#  The `Resolve` function should be
  **        called on the event system. This
  **        function runs through the queue of
  **        activated events and calls the
  **        callback functions associated with
  **        each. After resolving each event, it
  **        calls the destructor on the event
  **        (which is stored in the system's
  **        internal ScratchPad).
  **    .
  **  \see IEvent
  **  \see Event
  **  \see Event::Clone
  **  \see ScratchPad
  */
  class EventSys;
  
  /*! IEvent
  **  \brief
  **    Interface for the Event classes used by
  **    the event system.
  */
  class IEvent {
    //! allow EventSys access to internal code.
    friend class EventSys;
  public:
    virtual ~IEvent() = 0;
    IEvent(IEvent const&) = default;
    IEvent(IEvent &&) = default;
    IEvent & operator=(IEvent const&) = default;
    IEvent & operator=(IEvent &&) = default;
  protected:
    /*! Constructor
    **  \brief
    **    Internally used interface constructor.
    **  \param instant
    **    Details how the event should be handled.
    */
    IEvent(bool instant)
      : m_instant(instant)
    {}
    
    /*! CodeCount
    **  \brief
    **    Code generator function.
    **    Used internally.
    **  \return
    **    Returns the next code.
    */
    static size_t CodeCount() {
      static size_t count = 0;
      return count++;
    }
    
    
    /*! Code
    **  \brief
    **    Getter function for the Event's code.
    **    Used internally.
    **  \return
    **    Returns the Event's code.
    */
    virtual size_t Code() {
      return static_cast<size_t>(-1);
    }
    
    //! queued or instant handling
    bool m_instant;
  private:
  };
  
  // Required instantiation of destructor
  IEvent::~IEvent() {}

  /*! Event
  **  \brief
  **    CRTP Event base class to be used as a
  **    parent to all user-defined events.
  **  \tparam Type
  **    The inheriting class's type (CRTP).
  **  \par About:
  **    This is the Event class that all user-
  **    defined events should inherit from. The
  **    template parameter `Type` should be the
  **    type of the inheriting class, following
  **    the CRTP structure:
  **    + `class UserDef : public Event<UserDef>`
  **    .
  */
  template<typename Type>
  class Event: public IEvent {
    friend class EventSys;
  public:
    virtual ~Event() = 0;
    Event(Event const&) = default;
    Event(Event &&) = default;
    Event & operator=(Event const&) = default;
    Event & operator=(Event &&) = default;
    
    /*! Clone
    **  \brief
    **    Function to copy event to specified
    **    block of memory.
    **  \param block
    **    Pointer to destination memory.
    **  \par Overrides:
    **    Overriding Clone functions should
    **    perform any operations necessary for a
    **    deep copy of the event. Failure to do so
    **    may cause unintended side effects (read
    **    'bugs').
    */
    virtual void Clone(Type * block) = 0;
    
  protected:
    /*! Constructor
    **  \param instant
    **    Specifies how the event is handled.
    **    - true for instant
    **    - false for queue
    **    .
    */
    Event(bool instant = false)
      : IEvent(instant)
    {}
    
    /*! \see size_t IEvent::Code() */
    size_t Code() override {
      return m_code;
    }
    
    //! internally used identifier code.
    static const size_t m_code;
    
  private:
  };
  
  // Required instantiation of destructor
  template<typename Type>
  Event<Type>::~Event() {}

  // Instantiation of the static code
  template<typename Type>
  const size_t Event<Type>::m_code = CodeCount();

  class EventSys {
    /*! TriggerCall [member]
    **  \brief
    **    Function template to call listener's
    **    callback member function.
    **    Used internally.
    **  \tparam Listener
    **    The listener type.
    **  \tparam Trigger
    **    The trigger (event) type.
    **  \tparam Function
    **    The callback function.
    **  \param listener
    **    The pointer to the listening object.
    **  \param trigger
    **    The triggering event.
    **  \return
    **    Returns false if listener was null
    **    Otherwise returns result of callback
    **    This return value signifies if the
    **    EventSys should stop propogating the
    **    event (a minor optimization, allows
    **    for early exiting).
    */
    template<class Listener, class Trigger,
      bool (Listener::*Function)(Trigger *)>
    static bool TriggerCall(void * listener,
      IEvent * trigger)
    {
      // validate the listener
      if (listener != nullptr) {
        Listener * l =
          static_cast<Listener *>(listener);
        Trigger * t =
          dynamic_cast<Trigger *>(trigger);
        // call the member function
        return (l->*Function)(t);
      }
      return false;
    }
    
    /*! TriggerCall [static]
    **  \brief
    **    Function template to call a static
    **    callback function.
    **  \tparam Trigger
    **    The type of the triggering event.
    **  \tparam Function
    **    The static function to call.
    **  \param trigger
    **    The triggering event.
    **  \return
    **    Returns the result of calling Function.
    */
    template<class Trigger,
      bool (*Function)(Trigger *)>
    static bool TriggerCall(IEvent * trigger){
      return Function(
        dynamic_cast<Trigger *>(trigger)
      );
    }
    
    /*! Callback
    **  \brief
    **    Class used to wrap a callback function
    **    for storage and use.
    */
    class Callback {
    public:
      typedef bool (*MemCB)(void *, IEvent *);
      typedef bool (*StatCB)(IEvent *);
      Callback(Callback const& other) = default;
      ~Callback() = default;
      
      /*! Constructor [member]
      **  \param instance
      **    Class instance to bind to the
      **    callback.
      **  \param callback
      **    Pointer-to-member-function for the
      **    callback.
      */
      Callback(void * instance, MemCB callback)
        : m_member(instance),
          m_remove(false),
          m_memberCall(callback)
      {}
      
      /*! Constructor [static]
      **  \param callback
      **    Static callback function.
      */
      Callback(StatCB callback)
        : m_member(nullptr),
          m_remove(false),
          m_staticCall(callback)
      {}
      
      /*! operator()
      **  \brief
      **    Function call operator to call the
      **    callback function.
      **  \param trigger
      **    The triggering event.
      **  \return
      **    Returns the result of the callback
      **    function.
      */
      bool operator()(IEvent * trigger) const {
        // check that the callback is still valid
        if (m_remove) return false;
        // if the callback is static or member
        if (m_member != nullptr) {
          return m_memberCall(m_member, trigger);
        }
        else {
          return m_staticCall(trigger);
        }
      }
      
      /*! CheckFunction
      **  \brief
      **    Checks if function address is same as
      **    given pointer.
      */
      bool CheckFunction(void * against) const {
        return (m_void == against);
      }
      
      /*! CheckMember
      **  \brief
      **    Checks if the member address is the
      **    same as the given pointer.
      **  \param against
      **    The pointer to check against.
      **  \return
      **    Returns the result of the comparison.
      */
      bool CheckMember(void * against) const {
        return (m_member == against);
      }
      
      /*! Remove
      **  \brief
      **    Flags the Callback for removal.
      */
      void Remove() {
        m_remove = true;
      }
      
      /*! Removable
      **  \brief
      **    Checks if the Callback is flagged for
      **    removal.
      **  \return
      **    Returns the removal flag.
      */
      bool Removable() const {
        return m_remove;
      }
      
    private:
      //! pointer to the listener.
      void * m_member;
      //! flag for removal.
      bool m_remove;
      union {
        //! void pointer for comparison purposes
        void * m_void;
        //! pointer to member function.
        MemCB m_memberCall;
        //! pointer to static function.
        StatCB m_staticCall;
      };
    };
    
  public:
    EventSys(EventSys const&) = delete;
    EventSys(EventSys &&) = delete;
    EventSys & operator=(EventSys const&) = delete;
    EventSys & operator=(EventSys &&) = delete;
    
    /* Constructor */
    EventSys()
      : m_directory(),
        m_queue(),
        m_removalCodes(),
        m_pad(new ScratchPad<LTK_EVENTS_PADSIZE>)
    {}

    /*! Destructor
    **  \brief
    **    Destructs each event in the queue
    **    without resolving them.
    */
    ~EventSys() {
      // destruct each event in the queue
      std::for_each(m_queue.begin(),
        m_queue.end(),
        [&](IEvent * ev) {
          ev->~IEvent();
        }
      );
      delete m_pad;
    }
    
    /*! Get
    **  \brief
    **    Getter function for a default instance
    **    of the Event system.
    **  \return
    **    Returns a reference to the default
    **    EventSys.
    */
    static EventSys & Get() {
      static EventSys es;
      return es;
    }
    
    /*! Subscribe
    **  \brief
    **    Subscription function for objects with
    **    member callbacks.
    **  \tparam Listener
    **    The type of the listener class.
    **  \tparam Trigger
    **    The type of the triggering event.
    **  \tparam Function
    **    Pointer to the callback member function.
    **  \param listener
    **    Pointer to the listener object.
    */
    template<class Listener, class Trigger,
      bool (Listener::*Function)(Trigger *)>
    void Subscribe(Listener * listener) {
      // create the callback
      Callback cb(listener,
        &TriggerCall<Listener, Trigger, Function>
      );
      // push the callback onto the stack
      m_directory[Trigger::m_code].push_back(cb);
    }
    
    /*! Subscribe [static]
    **  \brief
    **    Subscription function for static
    **    callbacks.
    **  \tparam Trigger
    **    The triggering event.
    **  \tparam Function
    **    The callback function
    */
    template<class Trigger,
      bool (Function)(Trigger *)>
    void Subscribe() {
      // create the callback
      Callback cb =
        Callback(&TriggerCall<Trigger, Function>);
      // add the callback to the directory
      m_directory[Trigger::m_code].push_back(cb);
    }
    
    /*! Unsubscribe
    **  \brief
    **    Unsubscribes a member callback from an
    **    event.
    **  \warning
    **      This function only unsubscribes
    **      the first callback it finds. If an
    **      object subscribes multiple callbacks
    **      to the same event, it will only remove
    **      the first one it finds. In order to
    **      remove a specific subscription, the
    **      multiple subscriber must unsubscribe
    **      all callbacks and resubscribe all but
    **      the one it intends to remove.
    **  \tparam Trigger
    **    The type of the event.
    **  \param obj
    **    The object to unsubscribe.
    */
    template<class Trigger>
    void Unsubscribe(void * obj) {
      size_t code = Trigger::m_code;
      // find the Callback we want
      std::vector<Callback>::iterator unsub =
        std::find_if(m_directory[code].begin(),
          m_directory[code].end(),
          [&](Callback const& cb) {
            // check input against callback
            return (cb.CheckMember(obj) &&
              !(cb.Removable()));
          }
        );
      // if the unsubscribing object was valid
      if (unsub != m_directory[code].end()) {
        // flag it for removal
        (*unsub).Remove();
        // add the removal code
        m_removalCodes.insert(code);
      }
    }
    
    /*! Unsubscribe [static]
    **  \brief
    **    Unsubscribes a static callback from an
    **    event.
    **  \tparam Trigger
    **    The type of the triggering event.
    **  \tparam Function
    **    Pointer to the static function to
    **    unsubscribe.
    */
    template<class Trigger,
      bool(Function)(Trigger *)>
    void Unsubscribe() {
      size_t code = Trigger::m_code;
      
      std::vector<Callback>::iterator unsub =
        std::find_if(m_directory[code].begin(),
          m_directory[code].end(),
          [&](Callback const& cb) {
            return (cb.CheckMember(nullptr) &&
              cb.CheckFunction(
                reinterpret_cast<void *>(Function)
              )
            );
          }
        );
      
      if (unsub != m_directory[code].end()) {
        (*unsub).Remove();
        m_removalCodes.insert(code);
      }
    }
    
    /*! Activate
    **  \brief
    **    Activation for the inherited event.
    **  \param ev
    **    The event to activate.
    */
    template<typename EType>
    void Activate(EType & ev) {
      if (ev.m_instant) {
        // handle immediately
        Resolve(&ev);
      }
      else {
        // reallocate it on the ScratchPad
        EType * block = reinterpret_cast<EType *>(
          m_pad->Alloc(sizeof(EType)));
        // check that the scratchpad allocated
        if (block == nullptr) {
          // if not, resolve immediately
          Resolve(&ev);
          return;
        }
        // clone the event to the ScratchPad
        ev.Clone(block);
        
        // queue the copied event
        m_queue.push_back(
          static_cast<IEvent*>(block));
      }
    }
    
    /*! Resolve
    **  \brief
    **    Resolves all queued events.
    */
    void Resolve() {
      // resolve events in the queue
      std::for_each(m_queue.begin(),
        m_queue.end(),
        [&](IEvent * ev) {
          // resolve the current event
          Resolve(ev);
          // call destructor (event finished)
          ev->~IEvent();
        }
      );
      // clear the queue
      m_queue.clear();
      // handle the removals
      HandleRemoval();
      // reset the ScratchPad
      m_pad->Clear();
    }
    
  private:
    /*! HandleRemoval
    **  \brief
    **    Handles the removal of unsubscribed
    **    callbacks.
    */
    void HandleRemoval() {
      // for each of the codes set to remove
      std::for_each(m_removalCodes.begin(),
        m_removalCodes.end(),
        [&](size_t code) {
          // remove Callback if removable
          std::remove_if(
            m_directory[code].begin(),
            m_directory[code].end(),
            [&](Callback const& cb) {
              return cb.Removable();
            }
          );
        }
      );
    }
    // wow that's an intense set of closing braces
    
    /*! Resolve [internal]
    **  \brief
    **    Resolves a specific event.
    **  \param current
    **    The event to resolve.
    */
    void Resolve(IEvent * current) {
      // get the event's code
      size_t code = current->Code();
      // list of associated callbacks
      std::vector<Callback> list =
        m_directory[code];
      // loop through callbacks until done
      std::find_if(list.begin(), list.end(),
        [&](Callback & call) {
          bool result = call(current);
          return result;
        }
      );
    }
    
    //! map of active Callbacks
    std::unordered_map<size_t,
      std::vector<Callback> > m_directory;
    //! queue of events to handle
    std::vector<IEvent *> m_queue;
    //! vector of disconnecting codes
    std::set<size_t> m_removalCodes;
    //! ScratchPad allocator for events
    ScratchPad<LTK_EVENTS_PADSIZE> * m_pad;
  };

} // Luke's Tool Kit

#endif // !LTK_EVENTS_H

/* Authored by Luke Cardell
** Open source under GNU Public License
*/
