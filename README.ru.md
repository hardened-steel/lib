# Каналы (channels)
Каналы (channels, pipes) - это удобная абстракция для построения приложений работающих в
многопоточной среде. Они используются для передачи сообщений между потоками и,одновременно с этим, как средство синхронизации потоков. Я буду ссылаться на "Go-style channels" потому, что на мой взгляд, важная способность каналов в языке GO - это мультиплексирование каналов.

Реализации каналов на языке C++ конечно же есть, например в библиотеки boost::fibers, можно найти реализацию двух [видов каналов](https://www.boost.org/doc/libs/1_84_0/libs/fiber/doc/html/fiber/synchronization/channels.html). Так же там можно найти [описания способов](https://www.boost.org/doc/libs/1_84_0/libs/fiber/doc/html/fiber/when_any.html) мультиплексирования, правда не каналов, но и к ним можно применить подобную технику.

Реализация из boost не предлагает мультиплексирование каналов из коробки и никак не позиционирует себя на роль "Go-style channels", оно и понятно, представлен простой механизм передачи сообщений из одного ``fiber`` в другой. Предложенная техника мультиплексирования, которую можно применить и к каналам, это простая реализация "в лоб". Она состоит из запуска дополнительных промежуточных fibers по одному на задачу (или в нашем случае на канал). Это довольно затратно.

[Ещё одна реализация](https://github.com/Balnian/ChannelsCPP), у меня это была первая строчка поиска в google "go-style channels c++". Использует перегруженный оператор ``<<`` и ``>>``, есть мультиплексирование, но реализована через случайные опрос каналов в бесконечном цикле. А класс ``go::internal::ChannelBuffer`` содержит ошибку использования ``std::conditional_variable`` и поля ``std::atomic_bool is_closed;`` (обсудим это ниже).

Обе реализации использую циклический буфер для хранения передаваемых сообщений. Я бы хотел показать, что абстракция каналов - это нечто большее, чем просто циклический буфер с примитивами синхронизации. Попробую сформулировать требования для идеальной реализации каналов:
1) Мультиплексирование должно быть реализовано без использования "тяжёлых" сущностей.
    Под тяжелыми сущностями я подразумеваю следующее:
    * запуск дополнительных потоков, корутин (любых). 
    * не должно быть никаких "фоновых" или "сервисных" потоков.
    * выделение динамической памяти под каки-либо дополнительные структуры.

    Все что должно быть создано для реализации мультиплексирования, создается только в месте использования.
2) Блокировка потока в случае отсутствии данных в канале использует стандартные средства ОС (нет бесконечного цикла под капотом).
3) Блокировка должны быть до тех пор, пока данные не появяться в любом канале (простой round-robin не подойдёт).
4) Не вызывать блокировку, если в канале есть доступные данные.
   Должна быть возможность реализовать некоторые каналы, из которых можно прочитать данные без вызова блокировок ОС, или могут быть реализованы "поверх" lockfree структур данных. ([Polling](https://en.wikipedia.org/wiki/Polling_(computer_science)))
5) Поддержка интерфейса итераторов для чтения из канала:
    ```c++
    Channel<int>& ch = /*...*/;
    for(const auto& value: ch) {
        /*...*/
    }
    ```
6) Интерфейс канала допускает любую реализацию получения данных. В качестве простого примера представим себе канал, данные для которого генерируются "на лету":
    ```c++
    template<class T, class Fn>
    class ChannelGenerator: public Channel<T>
    {
        Fn generator;
        /*...*/
    };
    /*...*/
    auto channel = create_generator<int>(
        [counter = 0] { return counter++; }
    );
    for(const auto& value: channel) { // infinity loop!!!
        std::cout << "value from channel: " << value << std::endl;
    }
    // print:
    // value from channel: 0
    // value from channel: 1
    // value from channel: 2
    // ....
    ```

Добавлю и одно ограничение, по-умолчанию все каналы SPSC (single producer single consumer). Это ограничение сильно упрощает базовую реализацию.

## Простая реализация канала
Самая простая реализация MPMC канала с фиксированным буфером можно представить так:
```c++
template<class T, std::size_t N>
class Channel {
    CyclicBuffer<T, N> buffer;
    bool closed = false;

    mutable std::mutex              mt;
    mutable std::condition_variable cv;
public:
    using value_type = T;
};
```
Пример реализации метода recv:
```c++
bool recv(T& value)
{
    std::unique_lock lock{mt};
    cv.wait(lock, [this]{ return closed || !buffer.empty(); });
    if (closed) {
        return false;
    }
    value = buffer.pop();
    lock.unlock();
    cv.notify_all();
    return true;
}
```
Метод send реализуется схожим образом.
Теперь подумаем как можно добавить возможность мультиплексирования. Мы пока проигнорируем требования не вызывать блокировку, если в канале есть данные.
В данной реализация для синхронизации между потоками и ожидания читателя новых данных (если их ещё нет в буфере) использует пару из ``std::mutex`` и ``std::condition_variable``.
Для реализации мультиплексирования должен быть класс объединяющий наши каналы.
```c++
template<class ...Channels>
class ChannelSelect {
    Channels& ...channels;
public:
    ...
};
```
Теперь мы должны подумать какой интерфейс должен быть у нашего класса ``ChannelSelect``.
Напомню, что каналы могут передавать разные типы данных, поэтому простой интерфейс функии ``recv`` нам не подходит.
Оказывается объединять каналы можно по разному:

#### Способ 1
Можно вызвать ``callback`` для каждого канала как в библиотeке [ChannelsCPP](https://github.com/Balnian/ChannelsCPP), вот пример использования:
```c++
void fibonacci(Chan<int>& c, Chan<int>& quit)
{
    int x=0, y = 1;
    for (bool go = true; go;) {
        Select {
            Case {
                c << x,
                [&] {
                    int t = x;
                    x = y;
                    y += t;
                }
            },
            Case {
                quit,
                [&](auto v) {
                    cout << "quit" << endl;
                    go = false;
                }
            }
        };
    }
}
```
Тут ``Select`` и ``Case`` - это классы из библиотеки.

#### Способ 2
Метод ``recv`` возвращает комбинацию типов. И тут возможны два подварианта:
  1) Ожидать готовность всех объединённых каналов и возвращать ``std::tuple``.
     ```c++
     bool recv(std::tuple<typename Channels::value_type...>& value);
     ```
     Объединенный канал быдет считаться закрытым, если хотябы один из его подканалов закрыт.
  2) Ожидать готовность любого из объединенного канала и возвращать ``std::variant``
     ```c++
     bool recv(std::variant<typename Channels::value_type...>& value);
     ```
     Объединенный канал быдет считаться закрытым, если все из его подканалов закрыты.

Вариант 2 более гибкий ведь на его основе можно реализовать GO-style классы ``Select`` и ``Case``.

Простоя идея реализации мультиплексирования:
1) Нужно как-то сообщить каналу, что "над ним" кто-то есть. Раз мы не можем вызвать метод ``recv`` (мы заблокируемся), мы добавим в канал дополнительное поле - указатель (например на функцию), чтобы "писатель" после добавления данных разбудил "читателя".
2) В классе ``ChannelSelect`` есть свои пара ``std::mutex`` + ``std::condition_variable``.
3) Перед блокировкой класс ``ChannelSelect`` должен опросить все каналы на наличие в них данных. Таким образом в класс ``Channel`` нужно добавить метод ``poll``:
    ```c++
    bool poll() const
    {
        std::unique_lock lock{mt};
        return !buffer.empty();
    }
    ```
## Минус такого подхода
Такая архитектура позволяет удовлетворить все требования из списка кроме требования №4.

### Небольшое отступление
Часто я вижу ошибку при использования связки ``std::mutex`` + ``std::condition_variable`` - заменить некоторые (или все!) поля класса на атомарные переменные и изменять их без блокировки мьютекса. Действительно, метод [notify](https://en.cppreference.com/w/cpp/thread/condition_variable/notify_all) у класса ``std::condition_variable`` можно вызвать без блокировки. Но это не значит, что можно использовать ``std::condition_variable`` только для пробуждения другого потока!
Проилюстрирую проблему:
1) "Писатель" поместил данные (одно сообщение) в пустой циклический буфер (пусть он будет полностью lock-free) без блокировки мьютекса.
2) "Писатель" вызвал ``cv.notify_one()``. Больше данных "писатель" не предоставит в течении долгого времени (или никогда).
3) "Читатель" блокирует мьютекс и проверяет наличие данных в циклическом буфере, ничего не находит.
4) "Читатель" засыпает на ``cv.wait(...)``.

Представим, что хронологически порядок был таким: 3, 1, 2, 4. Таким образом, "читатель" заснет даже если есть данные в буфере, и разбудить его сможет только "писатель", когда он добавит следующую порцию данных. Но "писатель" мог закончить свою работу предоставив в буфер последние данные, тогда "читатель" заснет на всегда (deadlock).

Пример ошибки можно увидеть в [тут](https://github.com/Balnian/ChannelsCPP/blob/1ba5fd6b56d2983387356294e785b197c9b8e132/ChannelsCPP/ChannelBuffer.h#L84).

## Реализация на основе semaphore
Семафор лучше подходит для реализации канала. Свойства семафора позволяю реализовать lock-free канал. Если в канале есть данные, то блокироваться необязательно. Однако нам все равно нужно использовать его каждый раз во время чтения и записи в канал. Требования не блокироваться при наличии данных в канале перетекает в реализацию семафора. Чаще всего, реализация семафора - это тонкая обертка над семафором ОС, а он в свою очередь представляте собой некий дескриптор ОС и соответствующие системные вызовы. Конечно, можно положиться, что семафор реализован очень хорошо и не дергает ОС если засыпать не нужно. Но, я решил не использовать его в чистом виде, а вызывать методы семафора только в случае возможной блокировки.

# Обзор решения
## Объект события
Все, что нам нужно, это уведомить другой поток, что новые данные появились (или появилось место в буфере для записи). Для этого лучше всего подходит концепция событий. Объект событи обычно работает через механизм подписки на событие.
У нас есть ограничение SPSC (single producer single consumer), а значит у события может быть только один подписчик. А раз так, то мы можем представить объект события простым классом:
```c++
class Event
{
    std::atomic<std::uintptr_t> signal {0};
public:
    void emit() noexcept;
    bool poll() const noexcept;
    std::size_t subscribe(IHandler* handler) noexcept;
    std::size_t reset() noexcept;
};
```
Класс имеет одно единственно поле signal, которе хранит указатель на объект подписчика. Вот его интерфейс:
```c++
class IHandler
{
public:
    virtual void notify() noexcept = 0;
    virtual ~IHandler() noexcept {};
    virtual void wait() noexcept = 0;
};
```
``std::uintptr_t`` тут не просто так, дело в том, что наше событие может быть в 2 состояних: событие находится в сигнальном состоянии и нет. Для хранения этого сигнального состояния в отдельном атомарном поле я посчитал слишком расточительным, для этого достаточно одного бита, поэтому объединил его с указателем, все равно старшие биты указателей не используются.

Номер бита описан следующим образом:
```c++
// самый старший бит std::uintptr_t
constexpr std::uintptr_t bit = (std::uintptr_t(1) << (sizeof(std::uintptr_t) * CHAR_BIT - 1));
```

Что делают методы класса ``Event``:
* ``poll`` - опрос события, true - если событие произошло:
    ```c++
    bool Event::poll() const noexcept
    {
        return signal.load(std::memory_order_relaxed) & bit;
    }
    ```
* ``emit`` - устанавливает событие в сигнальное состояние
    ```c++
    void Event::emit() noexcept
    {
        if (!poll()) {
            if(auto handler = signal.fetch_or(bit); handler & ~bit) {
                reinterpret_cast<IHandler*>(handler)->notify();
            }
        }
    }
    ```
  Тут мы проверяем, что событие ещё не произошло, если это так, то читаем указатель с одновремнной установкой самого старшего бита, и вызываем у него ``notify()``.
  Дополнительная проверка в условии ``handler & ~bit`` защищает нас от конкуретного использования ``emit`` из разных потоков.
* ``subscribe`` - подписаться на событие, передача `` nullptr`` отписывает от события:
    ```c++
    std::size_t Event::subscribe(IHandler* handler) noexcept
    {
        if (signal.exchange(reinterpret_cast<std::intptr_t>(handler)) & bit) {
            return 1;
        }
        return 0;
    }
    ```
    Метод универсальный, мы можем передать валидный указатель для подписки, и ``nullptr`` для отписки. Во втором случае нам пригодтся возвращаемое значение, это количество "событий" которые успели произойти (или точно произойдут в будущем), до того как мы окончательно отписались. Если наш подписчик состоит из семафора (реализация по-умолчанию), то нам необходимо дождаться всех событий, прежде чем разрушить его.
* ``reset`` - сбрасывает событие в несигнальное состояние:
    ```c++
    std::size_t Event::reset() noexcept
    {
        if (signal.fetch_and(~bit) & bit) {
            return 1;
        }
        return 0;
    }
    ```
    Метод сбрасывает флаг события, аналогично методу ``subscribe`` возвращает количество событий которые успели произойти (или точно произойдут в будущем).

Все методы атомарные и не блокирующие, разве что метод ``emit`` вызовет ``notify``, который в свою очередь, задействует семафор.
Важное свойство объекта события - это то, что повторные вызовы ``emit`` будут только читать атомарную переменную и ничего больше не делать, пока событие вновь не будет сброшено.

Класс ``Handler`` - это простая реализация на основе семафора:
```c++
class Handler: public IHandler
{
    Semaphore semaphore;
public:
    void notify() noexcept override
    {
        semaphore.release();
    }
    void wait(std::size_t count = 1) noexcept override;
    {
        semaphore.acquire(count);
    }
};
```
Тут ничего сложного, разве что метод ``wait`` принимает некоторый счетчик. Это то самое количество ожидаемых событий, т.е. сумма того, что возвращают методы ``subscribe`` и ``reset``.
Попробую объяснить необходимость этих счётчиков. Иногда события переходят в сигнальное состояние, но "подписчик" не всегда вызывает ``wait``, так в семафоре копится свой внутренний счетчик. В момент удаления объекта подписчика, он конечно же "отписывается" от объекта события. Но при этом метод ``emit`` может все еще пытаться сделать ``notify``. Тут нам пригождаются этот отдельный счетчик событий. Перед разрушением, "подписчик" ожидает все события накопленные в семафоре, так мы обеспечиваем полную безопасность метода ``emit`` и разрушения класса ``Handler``.

### Подведем итог
Объект события работает в паре с любым объектом (или объектами), который может менять свое состояние и об этом надо уведомить другой поток. Пока всё выглядит так же как работа с парой ``std::mutex`` и ``std::condition_variable`` + ещё что-то, что меняет своё состояние.

Общий алгоритм работы с событием со стороны потока-писателя:
1) Изменить состояние (в нашем случае канал)
2) вызвать ``emit``
Важное отличие от ``std::condition_variable``, для изменения состояние не требуется блокировка, те это может быть, например, lock-free структура данных.

Общий алгоритм работы с событием со стороны потока-читатель выглядит так:
1) Создать объект ``Handler``.
2) Подписаться на событие.
3) Если нужно узнать произошло ли интересующее нас изменение:
   1) Опросить объект на предмет интересующих нас изменений.
   2) Если изменений нет, Вызвать ``reset``
   3) Ещё раз опросить объект на предмет интересующих нас изменений
   4) Если изменений нет, то вызвать ``wait``.
   5) Вызвать ``reset``.
   6) Перейти на шаг 3.
4) Отписаться от события.
Пункт 3, можно повторять сколько угодно раз. Объект ``Handler`` можно переиспользовать, поэтому к пункту 4 мы переходим когда выполнили всю работу.

Пункт 4 сложнее: не достаточно просто вызвать ``subscribe(nullptr)``, нам нужно учесть количества событий которые произошли, но для них не был вызван ``wait``. Для этого напишем класс упрощающий работу с объектами событий и подписчиками. Заодно, он защитит нас от ошибки "забыть отписаться":

```c++
template<class Event>
class Subscriber
{
    Event*      event;
    IHandler* handler;
    std::size_t count;
public:
    template<class Handler>
    Subscriber(Event& event, Handler& handler) noexcept
    : event(&event)
    , handler(&handler)
    , count(0)
    {
        event.subscribe(&handler);
    }

    Subscriber(Subscriber&& other) noexcept
    : event(other.event)
    , handler(other.handler)
    {
        other.event = nullptr;
    }

    void reset() noexcept
    {
        count += event->reset();
    }

    void wait() noexcept
    {
        handler->wait();
        count -= 1;
    }

    ~Subscriber() noexcept
    {
        if (event) {
            handler->wait(event->subscribe(nullptr) + count);
        }
    }
};
```
Мы обернули методы ``wait`` у класса ``Handler`` и метод ``reset`` у класса ``Event``. Теперь мы никогда не забудем отписаться, и сделаем это всегда правильно.

## Мультиплексирование событий
Как нам теперь объединить несколько объектов события в один? Довольно просто, но нужно оговориться, что такой тип события переходит в сигнальное состояние тогда, когда хотябы одно подсобытие находится в сигнальном состоянии.

Вот его код:
```c++
template<class ...Events>
class EventMux
{
    std::tuple<Events&...> events;
public:
    constexpr EventMux(Events& ...events) noexcept
    : events{events...}
    {}

    std::size_t subscribe(IHandler* handler) noexcept
    {
        return subscribe(handler, std::make_index_sequence<sizeof...(Events)>{});
    }
    std::size_t reset() noexcept
    {
        return reset(std::make_index_sequence<sizeof...(Events)>{});
    }
    bool poll() const noexcept
    {
        return poll(std::make_index_sequence<sizeof...(Events)>{});
    }
private:
    template<std::size_t ...I>
    std::size_t subscribe(IHandler* handler, std::index_sequence<I...>) noexcept
    {
        return (std::get<I>(events).subscribe(handler) + ...);
    }
    template<std::size_t ...I>
    std::size_t reset(std::index_sequence<I...>) noexcept
    {
        return (std::get<I>(events).reset() + ...);
    }
    template<std::size_t ...I>
    bool poll(std::index_sequence<I...>) const noexcept
    {
        return (std::get<I>(events).poll() || ...);
    }
};
template<typename... Events>
EventMux(Events& ...events) -> EventMux<Events...>;
```
Все методы мы просто дилегируем подконтрольным событиям. В случае ``poll`` для результата мы используем оператор логического "или". В случае ``reset`` и ``subscribe`` просто суммируем возвращаемые значения.
Единственно отличие тут, это отсутствие метода ``emit``, для этого класса он не нужен.

## Каналы
Каналы могут быть для чтения или для записи, либо и то и другое. Интерфейсы у них очень похожы, поэтому для схожих методов для каналов чтения я использую префикс "r" (recv), для записи - префикс "s" (send).
Общий интерфейс канала для чтения можно представить так:
```c++
template<class T>
class IChannelInterface
{
public:
    using Type = T;
    using REvent = Event;
public:
    bool rpoll() const noexcept;
    Type urecv();

    void close() noexcept;
    bool closed() const noexcept;

    REvent& revent() const noexcept;
};
```
* ``rpoll`` - опросить канал на предмет новых сообщений. ``true`` - если канал готов к чтению, ``false`` если в канале ничего нет.
* ``urecv`` - прочитать (или получить) следующее сообщение. Суффикс "u" - обозначает "unsafe", этот метод нельзя вызывать, если предварительно не был вызван ``rpoll``, который вернул ``true``.
* ``close`` - закрыть канал, сообщения которые ещё остались в канале, остануться доступными для чтения. После вызова этого метода, ``rpoll`` всё ещё может возвращать ``true``, если что-то осталось не прочитанным.
* ``closed`` - думаю тут все понятно.
* ``revent`` - возвращает ссылку на связанный объект события.
В дополнении к этим методам, интерфейс должен предоставить два типа:
* ``Type`` - тип сообщения. ``urecv`` должен возвращать объект именно этого типа.
* ``REvent`` - тип привязанного объекта события. ``revent`` должен возвращать ссылку на этот тип.

В интерфейсе не используются виртуальные функции потому, что в данной реализации используется (в основном) статический полиморфизм. Если есть статический полиморфизм, то вариант с виртуальными функциями несложно реализовать.

Теперь интерфейс канала для записи, он почти ничем не отличается:
```c++
template<class T>
class OChannelInterface
{
public:
    using Type = T;
    using SEvent = Event;
public:
    bool spoll() const noexcept;
    void usend(T value);

    void close() noexcept;
    bool closed() const noexcept;

    SEvent& sevent() const noexcept;
};
```

Оба интерфейса достаточно "низкоуровневые" для прямого использования, просто так ``usend/urecv`` не вызвать, нужно убедиться что ``spoll/rpoll`` вернул ``true``, а если нет, то надо работать с объектом события, ссылку на который возвращает ``sevent/revent``. Обычно мы просто хотим вызвать что-то вроде ``channel.send(value)``, и заблокироваться если в канале недостаточно места.
Сделаем класс-helper для этого:
```c++
template<class Channel>
class OChannel
{
public:
    template<class Event, class TChannel>
    static bool swait(Subscriber<Event>& subscriber, const TChannel& channel) noexcept
    {
        subscriber.reset();

        bool closed = channel.closed();
        bool ready = channel.spoll();

        while(!closed && !ready) {
            subscriber.wait();
            subscriber.reset();

            closed = channel.closed();
            ready = channel.spoll();
        }
        return ready;
    }

    template<class T>
    void send(T&& value)
    {
        auto& self = *static_cast<Channel*>(this);
        if (self.spoll()) {
            return self.usend(std::forward<T>(value));
        }
        Handler handler;
        Subscriber subscriber(self.sevent(), handler);

        if (swait(subscriber, self)) {
            self.usend(std::forward<T>(value));
        }
    }
};
```
Тут можно увидеть ``auto& self = *static_cast<Channel*>(this);`` и наследование от параметра шаблона, а сам параметр называется ``Channel``. Все верно это CRTP.
Применять его надо так:
```c++
template<class T>
class OChannelInterface: public OChannel<OChannelInterface<T>>
{
public:
    ...
};
```
Готово, теперь у нас есть удобный метод ``send``. Для канала-читателя есть аналогичный класс. Заметим, что метод хоть и удобный, но неэффективный, он каждый раз на стеке создает объекты ``Handler`` и ``Subscriber``, и использует их одноразово.

### Range-based for для канала
Для выполнения требования №5 необходимо реализовать классы итераторов. Я решил сделать это отдельной сущностью ``IRange`` и свободной функцией ``irange``. Класс и функция шаблонные и работают с любыми каналами для чтения. Реализация неидеальная, представлю её целиком:
```c++
template<class Channel>
class IRange
{
    friend class Iterator;
private:
    using Value = std::remove_cv_t<std::remove_reference_t<typename Channel::Type>>;

    Channel& channel;
    Handler  handler;
    Subscriber<typename Channel::REvent> subscriber;

    // Хранилище для временного объекта
    std::aligned_storage_t<sizeof(Value), alignof(Value)> value;
public:
    class Iterator
    {
        IRange *range;
    public:
        using iterator_category = std::input_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = typename Channel::Type;
        using pointer           = std::remove_reference_t<value_type>*;
        using reference         = std::remove_reference_t<value_type>&;
    private:
        void next()
        {
            if (!Channel::rwait(range->subscriber, range->channel)) {
                range = nullptr;
            } else {
                auto ptr = reinterpret_cast<value_type*>(&range->value);
                new(ptr) value_type(range->channel.urecv());
            }
        }
    public:
        Iterator(IRange *range) noexcept
        : range(range)
        {
            if (range) {
                next();
            }
        }
        ~Iterator() noexcept
        {
            if(range) {
                auto ptr = reinterpret_cast<value_type*>(&range->value);
                std::destroy_at(ptr);
            }
        }
        auto& operator*() const noexcept
        {
            auto ptr = reinterpret_cast<value_type*>(&range->value);
            return *ptr;
        }
        Iterator& operator++()
        {
            auto ptr = reinterpret_cast<value_type*>(&range->value);
            std::destroy_at(ptr);

            next();
            return *this;
        }
        friend bool operator==(const Iterator& a, const Iterator& b) noexcept
        {
            return a.range == b.range;
        }
        friend bool operator!=(const Iterator& a, const Iterator& b) noexcept
        {
            return a.range != b.range;
        }
    };
public:
    constexpr IRange(Channel& channel) noexcept
    : channel(channel)
    , subscriber(channel.revent(), handler)
    {}

    auto begin()
    {
        return Iterator(this);
    }
    auto end()
    {
        return Iterator(nullptr);
    }
public:
    IRange(const IRange& range) = delete;
    IRange& operator=(const IRange& range) = delete;
};

template<class Channel>
auto irange(Channel& channel)
{
    return IRange<Channel>(channel);
}
```
Отмечу несколько моментов:
1) Класс ```IRange`` хранит объекты ``Handler`` и ``Subscriber``. Это значит, что подписка на событие происходит в самом начале, а отписка только один раз в конце работы с итераторами.
2) Где нужно хранить сообщение, чтобы разыменование итератора возвращало ссылку на сообщение.
3) Конструктор и деструктор сообщения вызывается только в определённые моменты, нет необходимости в дополнительных переменных или использовать класс ``std::optional`` для этого.

### Мультиплексирование каналов
Теперь у нас есть все необходимое для мультиплексирования каналов. Как было отмечено выше, мультиплексировать можно двумя способами.

#### Способ с готовностью любого канала
Для мультиплексирования по схеме "или" напишем шаблонный класс:
```c++
template<class ...Channels>
class IChannelAny
    : public IChannel<IChannelAny<Channels...>> // CRTP
{
public:
    using Type = std::variant<typename Channels::Type...>;
    using REvent = EventMux<typename Channels::REvent&...>;
private:
    std::tuple<Channels&...> channels;
    mutable REvent           events;
    mutable std::size_t      current = 0;
public:
    constexpr IChannelAny(Channels&... channels) noexcept
    : channels{channels...}
    , events{channels.revent()...}
    {
    }
    bool rpoll() const noexcept;
    bool closed() const noexcept;
    void close() noexcept;
    REvent& revent() const noexcept
    {
        return events;
    }
    Type urecv();
private:
    ...
};

template<typename... Channels>
IChannelAny(Channels&... channels) -> IChannelAny<Channels...>;
```

Класс просто сохраняет ссылки на объединяемые каналы в список ``std::tuple``. У него есть свой объект события на основе класса ``EventMux`` и счётчик для реализации round-robin.
Возвращаемый тип - ``std::variant<typename Channels::Type...>``. Все каналы могут иметь разные типы сообщений. Если типы повторяются, то все равно будут представлены в ``std::variant`` под своим индексом.
Реализация методов простые:
* ``close`` - вызывает ``close`` для каждого подканала.
* ``closed`` - вызывает ``closed`` для каждого подканала и возвращает ``true`` если все из них вернули ``true``.
* ``rpoll`` - вызывает ``rpoll`` для каждого подканала, но запоминает на каком остановился. Вызова ``rpoll`` в цикле использует массив указателей на функции для стирания типа:
    ```c++
    template<std::size_t ...I>
    bool rpoll(std::index_sequence<I...>) const noexcept
    {
        using PollFn = bool (*)(const IChannelAny*);
        static const std::array<PollFn, sizeof...(Channels)> poll {
            [](const IChannelAny* channel) {
                return std::get<I>(channel->channels).rpoll();
            }...
        };

        for (std::size_t i = 0; i < sizeof...(Channels); ++i) {
            if (poll[current](this)) {
                return true;
            }
            current += 1;
            if (current == sizeof...(Channels)) {
                current = 0;
            }
        }
        return false;
    }
    ```
* ``urecv`` - вызывает ``urecv`` у канала, функция ``rpoll`` которого вернула ``true``. Тут тоже используется массив указателей для стирания типа:
    ```c++
    template<std::size_t ...I>
    Type urecv(std::index_sequence<I...>)
    {
        using RecvFn = Type (*)(IChannelAny*);
        static const std::array<RecvFn, sizeof...(Channels)> recv {
            [](IChannelAny* channel) {
                return Type(std::in_place_index<I>, std::get<I>(channel->channels).urecv());
            }...
        };
        return recv[current](this);
    }
    ```
Класс ``IChannelAny`` соответсвует интерфейсу ``IChannelInterface``, значит его также можно мультиплексировать с другими каналами.

#### Способ с готовностью всех каналов
Для мультиплексирования по схеме "и" напишем шаблонный класс:
```c++
template<class ...Channels>
class IChannelAll: public IChannel<IChannelAll<Channels...>>
{
public:
    using Type = std::tuple<typename Channels::Type...>;
    using REvent = Event::Mux<typename Channels::REvent&...>;
private:
    std::tuple<Channels&...>                 channels;
    mutable REvent                           events;
    mutable std::bitset<sizeof...(Channels)> states;
public:
    constexpr IChannelAll(Channels&... channels) noexcept
    : channels{channels...}
    , events{channels.revent()...}
    {
    }
    bool rpoll() const noexcept;
    bool closed() const noexcept;
    void close() noexcept;
    REvent& revent() const noexcept
    {
        return events;
    }
    Type urecv();
private:
    ...
};

template<typename... Channels>
IChannelAll(Channels&... channels) -> IChannelAll<Channels...>;
```
Почти всё тоже самое, что у класс ``IChannelAny``. Вместо счёчика используется поле std::bitset для каждого канала по биту - небольшая оптимизация, чтобы повторно не опрашивать каналы. Возвращаемый тип - ``std::tuple<typename Channels::Type...>``.
* ``close`` - вызывает ``close`` для каждого подканала. Тут все так же.
* ``closed`` - вызывает ``closed`` для каждого подканала и возвращает ``true`` если хотябы один из них вернул ``true``.
* ``rpoll`` - вызывает ``rpoll`` для каждого подканала, результат сохраняет в битовое поле.
    ```c++
    template<std::size_t ...I>
    bool rpoll(std::index_sequence<I...>) const noexcept
    {
        if(states.all()) {
            return true;
        }
        ((states[I] = states.test(I) || std::get<I>(channels).rpoll()), ...);
        return states.all();
    }
    ```
* ``urecv`` - вызывает ``urecv`` у всех каналов и объединяет результаты в кортеж:
    ```c++
    template<std::size_t ...I>
    Type urecv(std::index_sequence<I...>)
    {
        states.reset();
        return std::make_tuple(std::get<I>(channels).urecv()...);
    }
    ```
Класс ``IChannelAll`` тоже соответсвует интерфейсу ``IChannelInterface``, значит его тоже можно мультиплексировать с другими каналами.

Используя ``structured binding``, чтение из ``IChannelAll`` можно организовать следующим образом:
```c++
lib::IChannelAll channels_ab(channel_a, channel_b);
for(const auto& [a, b]: lib::irange(channels_ab)) {
    ...
}
```

# Итоги

Каналы (и события) реализуют паттерн компоновщика: мультиплексированные каналы, в свою очередь тоже могут быть мультиплексированы и так далее, все будет работать с любым "уровнем вложенности". Все требования к реализации удовлетворены: никакой динамической памяти, никаких дополнительных, сервисных (фоновых) или промежуточных потоков, корутин и тд. Мы можем обернуть в канал lock-free очередь или сделать тонкую обертку над функцией-генератором или чего-то ещё.

Архитектурные возможности библиотеки позволяют реализовать каналы и их мультиплексирование на любой вкус.
Приведу пару примеров.

## AggregateChannel
Что если нам нужно объединить несколько каналов по схеме "или", но все каналы передают один тип сообщения? Или разные типы сообщений, но все они могут быть преобразованы в один общий тип. При этом, читателю не важно из какого именно канала пришло сообщение. Можно использовать обычный ``IChannelAny``, но итоговый тип у нас будет ``std::variant``. Для такого объединения можно написать отдельный класс:
```c++
template<class T, class ...Channels>
class AggregateChannel: public IChannel<AggregateChannel<T, Channels...>>
{
public:
    using Type = T;
    using REvent = Event::Mux<typename Channels::REvent& ...>;
private:
    std::tuple<Channels&...> channels;
    mutable REvent           events;
    mutable std::size_t      current = 0;
public:
    AggregateChannel(Channels& ...channels) noexcept
    : channels{channels...}
    , events(channels.revent()...)
    , current(0)
    {}
public:
    bool rpoll() const noexcept;
    bool closed() const noexcept;
    void close() noexcept;
    REvent& revent() const noexcept;
    Type urecv();
private:
    ...
};

template<class ...Channels>
AggregateChannel(Channels& ...channels) -> AggregateChannel<std::common_type_t<typename Channels::Type ...>, Channels...>;
```
Логика работы этого класса практически идентична класса ``IChannelAny``, разница лишь в создаваемом типе сообщения.

## BroadCastChannel
Канал для записи, который рассылает копии сообщений в другие каналы? Реализация тривиальна:
```c++
template<class T, class ...Channels>
class BroadCastChannel: public OChannel<BroadCastChannel<T, Channels...>>
{
public:
    using Type = T;
    using SEvent = Event::Mux<typename Channels::SEvent& ...>;
private:
    std::tuple<Channels&...> channels;
    mutable SEvent events;
public:
    BroadCastChannel(Channels& ...channels) noexcept
    : channels{channels...}
    , events(channels.sevent()...)
    {}
public:
    SEvent& sevent() const noexcept;
    void usend(T value);
    bool spoll() const noexcept;
    void close() noexcept;
    bool closed() const noexcept;
private:
    ...
    template<std::size_t ...I>
    void usend(T value, std::index_sequence<I...>)
    {
        (std::get<I>(channels).usend(value), ...);
    }
    ...
};

template<class ...Channels>
BroadCastChannel(Channels& ...channels) -> BroadCastChannel<std::common_type_t<typename Channels::Type ...>, Channels...>;
```
По-умолчанию входной тип вычисляется так ``std::common_type_t<typename Channels::Type ...>``, но можно указать любой другой, главное, чтобы он мог быть преобразован в типы ``typename Channels::Type``.

# Возможное дальнейшее развитие библиотеки

## События ОС

Основа реализации - это класс события и подписчика. Событие может перейти в сигнальное состояние только через вызов метода ``emit``, это ограничение не позволяет нам обобщить такие сущности как файловые дескрипторы или сокеты. Что если мы хотим обернуть взаимодействие по сети в абстракцию канала? Саздавать сервисный или промежуточный поток - неприемлимо. Что можно с этим сделать в будущем:
* Добавить новый тип сообщений - тонкая обёртка над объектом ОС. У такого класса не будет метода ``emit``.
* Сделать частичную специализацию шаблона класса мультиплексирования событий, если мы видим в списке типов событий "событие ОС".
* Подписчик, тоже меняет свой тип, либо адаптивно подстраивается под общий тип события.
* Объединяем несколько событий ОС для использования механизма мультиплексирования из ОС (например под системный вызов poll в linux).
* "Обычные" события тоже объединяются, но например, в [eventfd](https://man7.org/linux/man-pages/man2/eventfd.2.html) со своим дескриптором. Метод ``emit`` у таких событий будет пробуждать поток через eventfd.
Пока общую картину сложно обрисовать, но интуиция подсказывает, что это более чем возможно. Тут ещё сложности с каналами с динамическим полиморфизмом, например, какой тип сообщения будет возвращать методы ``revent/sevent``?

## Корутины c++20

Мы можем реализовать классы преобразующие один тип сообщения в другой через пользовательскую функцию. Реализация тривиальна, но это работает если преобразование происходит один к одному. Что если мы хотим преобразование многие к одному (или один во многие). Например из некоторого канала приходят буферы байтов что-то вроде ``std::span<std::byte>`` или ``std::string_view``, мы можем написать функцию, которая парсит такие данные и возвращает другой объект, например json структуру. Хорошо если у нас такой парсер поддерживает потоковый интерфейс, например [boost::json::stream_parser](https://www.boost.org/doc/libs/1_84_0/libs/json/doc/html/json/ref/boost__json__stream_parser.html).
В ином случае, нам нужен будет промежуточный поток или stackfull корутина, задача которого будет только в одном: парсить множество объектов в один и передавать дальше.

В c++20 есть базовая поддержка stackless корутин. Интеграция в библиотеку позволит просто реализовывать сложные преобразователи с минимальными накладными расходами. Также генераторы (требование №6) станут более органичными, одна c++ корутина может описать бесконечные и конечные генераторы и все это можно завернуть в интерфейс канала. При этом не нужен будет пулл потоков для выполнения корутин, все они могут выполняться в рамках одного потока, который в итоге читает (или пишет в) канал.

## Output итераторы и мультиплексирование каналов разной направленности

На данный момент каналы для записи не поддерживают итераторы. Мы можем их мультиплексировать, но не смешивать с каналами для чтения. По крайне мере, пока такой функционал не разработан. Идея в том, чтобы можно было написать как-то так:
```c++
    for (auto& [input, output]: lib::IChannelAll{channel_in, channel_out}) {
        ouput = proccess(input);
    }
```
Таким образом тело цикла будет выполняться только тогда, когда оба канала готовы: во входящем канале есть новое сообщение, а в выходном есть место для отправки.

## Динамическое мультиплексирование

Мы можем мультиплексировать сколь угодно много каналов в один, но менять количество каналов динамически нельзя. Это можно исправить добавив отдельный класс мультиплексора событий и каналов, который принимает на вход, например ``std::vector`` или ``std::span`` с событиями и каналами соответственно. Такой код сможет работать только с каналами с одним типом сообщений, и сами каналы должны быть с динамическим полиморфизмом, что логично.

## Оптимизация вызовов rpoll/spoll

Перед каждым вызовом метода ``usend/urecv`` мы должны убедиться что канал доступен вызвав ``rpoll/spoll``.
Можно уменьшить количество вызовов ``rpoll/spoll`` если они вместо ``bool`` будут возвращать, количество доступных сообщений для чтения/записи. Это количество надо будет где-то хранить, так мы выиграем в производительности немного увеличив потребление памяти.

