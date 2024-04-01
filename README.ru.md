# Каналы (channels)
Каналы (channels, pipes) – это удобная абстракция для построения приложений, работающих в
многопоточной среде.
Они используются для передачи сообщений между потоками и, одновременно с этим, как средство синхронизации потоков.
Я буду ссылаться на "Go-style channels", т.к. на мой взгляд, важная особенность каналов в языке GO – это возможность их мультиплексировать.

Реализации каналов на языке C++ конечно же есть, например, в библиотеке boost::fibers, можно найти реализацию двух [видов каналов](https://www.boost.org/doc/libs/1_84_0/libs/fiber/doc/html/fiber/synchronization/channels.html).
В документации boost::fibers можно найти [описания способов](https://www.boost.org/doc/libs/1_84_0/libs/fiber/doc/html/fiber/when_any.html) мультиплексирования, правда не самих каналов, но и к ним можно применить подобную технику.

Реализация из boost не предлагает мультиплексирование каналов "из коробки" и никак не позиционирует себя на роль "Go-style channels", оно и понятно, представлен простой механизм передачи сообщений из одного ``fiber`` в другой.
Предложенная техника мультиплексирования, которую можно применить и к каналам – это простая реализация "в лоб".
Она состоит из запуска дополнительных промежуточных ``fibers`` по одному на задачу (или в нашем случае, на канал). Применение ``fibers`` для задачи мультиплексирования довольно затратно.

Ещё одна реализация, моя первая строчка поиска в google "go-style channels C++" выдала [такой результат](https://github.com/Balnian/ChannelsCPP).
Библиотека использует перегруженный оператор ``<<`` и ``>>``, есть мультиплексирование, но выполнена через случайный опрос каналов в бесконечном цикле.
А класс ``go::internal::ChannelBuffer`` содержит ошибку использования ``std::conditional_variable`` и поля ``std::atomic_bool is_closed;`` (обсудим это ниже).

Обе реализации используют циклический буфер для хранения передаваемых сообщений.
Продемонстрирую на примерах, что абстракция каналов – это нечто большее, чем просто циклический буфер с примитивами синхронизации.
Сформулирую требования для идеальной реализации каналов:
1) Мультиплексирование реализовано без использования "тяжёлых" сущностей.
    Под тяжёлыми сущностями я подразумеваю следующее:
    * запуск дополнительных потоков, корутин (любых);
    * отсутствие каких-либо "фоновых" или "сервисных" потоков;
    * отсутствие использования динамической памяти.

    Дополнительные структуры данных, которые обеспечивают мультиплексирование, должны создаваться только по необходимости.

2) Блокировка потока возникает только в случае отсутствия данных в канале и использует стандартные средства ОС (нет бесконечного цикла "под капотом"). Таким образом, канал может использовать lockfree-контейнер, при этом, объекты синхронизации не задействуются, пока в канале есть сообщения.
3) Блокировка длится до тех пор, пока данные не появятся в любом канале (простой ``round-robin`` не подходит).
4) Поддержка интерфейса итераторов для чтения из канала:
    ```c++
    Channel<int>& ch = /*...*/;
    for(const auto& value: ch) {
        /*...*/
    }
    ```
5) Интерфейс канала допускает любую реализацию получения данных. В качестве простого примера представим себе канал, данные для которого, генерируются "на лету":
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

Добавлю и одно ограничение, по-умолчанию все каналы SPSC (single producer single consumer). Оно упрощает базовую реализацию.
Пункт №2 требует отдельного пояснения. Идея заключается в следующем: объединить [polling](https://en.wikipedia.org/wiki/Polling_(computer_science))-подход с блокировками. У polling-подхода есть преимущество в эффективности и минимизации задержек, но его недостаток состоит в том, что при отсутствии данных бесполезно тратится процессорное время на постоянный опрос. Объединение подходов работает так: когда в канале (каналах) есть сообщения – использовать polling-подход, когда же сообщений нет, то использовать блокировку.

## Простая реализация канала
Самую простую реализацию MPMC (multiple producer multiple consumer) канала с буфером фиксированного размера можно представить так:
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
Пример реализации метода ``recv``:
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
Метод ``send`` реализуется схожим образом.
Теперь подумаем над тем, как можно добавить возможность мультиплексирования. Мы пока проигнорируем требование не вызывать блокировку, если в канале есть данные.
В данной реализации используется пара из ``std::mutex`` и ``std::condition_variable`` для синхронизации между потоками.
[TODO] Нам нужен класс, объединяющий наши каналы:
```c++
template<class ...Channels>
class ChannelSelect {
    std::tuple<Channels&...> channels;
public:
    ...
};
```
Следующий шаг – проработать интерфейс у класса ``ChannelSelect``.
Напомню, что каналы могут передавать разные типы данных, поэтому простой интерфейс функции ``recv`` нам не подходит.
В действительности, объединять каналы можно по-разному.

1) Вызов ``callback`` для каждого канала, как в библиотeке [ChannelsCPP](https://github.com/Balnian/ChannelsCPP).
    Пример использования:
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
    В примере, ``Select`` и ``Case`` – это классы из библиотеки.

2) Метод ``recv`` возвращает комбинацию типов. Возможны два подварианта:
     1) Ожидать готовность всех объединённых каналов и возвращать ``std::tuple``.
        ```c++
        bool recv(std::tuple<typename Channels::value_type...>& value);
        ```
        Объединённый канал считается закрытым, если хотя бы один из его подканалов закрыт (схема "и").
     2) Ожидать готовность любого из объединённого канала и возвращать ``std::variant``
        ```c++
        bool recv(std::variant<typename Channels::value_type...>& value);
        ```
        Объединённый канал считается закрытым, если все из его подканалов закрыты (схема "или").

    Подвариант 2 позволяет реализовать "GO-style" классы ``Select`` и ``Case``.

Простая идея реализации мультиплексирования:
1) Нужно как-то сообщить каналу, что "над ним" кто-то есть. Мы не можем вызвать метод ``recv``, т.к. заблокируемся. Добавим в канал дополнительное поле – указатель (например, на функцию), чтобы "писатель" разбудил "читателя" после добавления данных.
2) В классе ``ChannelSelect`` есть своя пара ``std::mutex`` + ``std::condition_variable``.
3) Добавим метод ``poll`` в класс ``Channel``, чтобы принять решение о блокировке класса ``ChannelSelect`` (если данных нет, то блокируемся):
    ```c++
    bool poll() const
    {
        std::unique_lock lock{mt};
        return !buffer.empty();
    }
    ```
## Минус такого подхода
Такая архитектура позволяет удовлетворить все требования из списка, кроме требования №2.

### Небольшое отступление
Часто я вижу ошибку при использовании связки ``std::mutex`` + ``std::condition_variable`` – заменить некоторые (или все!) поля класса на атомарные переменные. Действительно, метод [notify](https://en.cppreference.com/w/cpp/thread/condition_variable/notify_all) у класса ``std::condition_variable`` можно вызвать без блокировки. Но это не значит, что можно использовать ``std::condition_variable`` только для пробуждения другого потока!
Проиллюстрирую проблему:
1) "Писатель" поместил данные (одно сообщение) в пустой циклический буфер (пусть он будет lock-free) без захвата мьютекса.
2) "Писатель" вызвал ``cv.notify_one()``. Больше данных "писатель" не предоставит в течение долгого времени (или никогда).
3) "Читатель" захватывает мьютекс и проверяет наличие данных в циклическом буфере, ничего не находит.
4) "Читатель" засыпает на ``cv.wait(...)``.

Представим, что хронологический порядок такой: 3, 1, 2, 4. Следовательно, "читатель" заснёт, даже если есть данные в буфере, и разбудить его сможет только "писатель", когда он добавит следующую порцию данных. Но "писатель" мог закончить свою работу, предоставив в буфер последние данные, тогда "читатель" заснет навсегда (deadlock), при этом в буфере остались необработанные данные.

Пример ошибки можно увидеть [тут](https://github.com/Balnian/ChannelsCPP/blob/1ba5fd6b56d2983387356294e785b197c9b8e132/ChannelsCPP/ChannelBuffer.h#L84).

## Реализация на основе semaphore
Семафор лучше подходит для реализации канала. Свойства семафора позволяют реализовать lock-free канал. Если в канале есть данные, то блокироваться необязательно. Однако, нам всё равно нужно использовать его каждый раз во время записи/чтения в/из канал/а. Требование "не блокироваться" (при наличии данных в канале) перетекает в реализацию семафора. Чаще всего, реализация семафора – это тонкая обертка над семафором ОС, а он, в свою очередь, представляет собой некий дескриптор ОС и соответствующие системные вызовы. Конечно, можно положиться на тот исход, что семафор реализован достаточно хорошо и не использует системный вызов ОС, если блокировка не требуется. Но, я решил не использовать его в чистом виде, а вызывать методы семафора только в случае необходимости.

# Обзор решения
## Объект события
Всё, что нам нужно, это уведомить другой поток о том, что новые данные появились (или появилось место в буфере для записи). Для этого лучше всего подходит концепция событий. Объект события, обычно, работает через механизм подписки на событие.

У нас есть ограничение SPSC (single producer single consumer), а значит, у события может быть только один подписчик.
Представим объект события в виде простого класса:
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
У класса есть единственное поле – ``signal``, которое хранит указатель на объект подписчика. Вот его интерфейс:
```c++
class IHandler
{
public:
    virtual void notify() noexcept = 0;
    virtual ~IHandler() noexcept {};
    virtual void wait(std::size_t count = 1) noexcept = 0;
};
```
``std::uintptr_t`` тут не просто так. Обозначим два состояния для события: сигнальное и несигнальное. Хранение сигнального состояния в отдельном атомарном поле слишком расточительно, для этой цели достаточно одного бита. Так как старшие биты указателей не используются, объединим бит-состояния с указателем.

Номер бита описан следующим образом:
```c++
// самый старший бит std::uintptr_t
constexpr std::uintptr_t bit = (std::uintptr_t(1) << (sizeof(std::uintptr_t) * CHAR_BIT - 1));
```

Что делают методы класса ``Event``:
* ``poll`` – опрос события, true – если событие произошло:
    ```c++
    bool Event::poll() const noexcept
    {
        return signal.load(std::memory_order_relaxed) & bit;
    }
    ```
* ``emit`` – устанавливает событие в сигнальное состояние:
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
  Тут мы проверяем, что событие ещё не произошло и, если это так, то читаем указатель с одновременной установкой самого старшего бита. Далее, вызываем у него ``notify()``.
  Дополнительная проверка в условии ``handler & ~bit`` позволяет вызывать ``emit`` из разных потоков.
* ``subscribe`` – подписаться на событие, передача ``nullptr`` отписывает подписчика от этого события:
    ```c++
    std::size_t Event::subscribe(IHandler* handler) noexcept
    {
        if (signal.exchange(reinterpret_cast<std::intptr_t>(handler)) & bit) {
            return 1;
        }
        return 0;
    }
    ```
    Метод универсальный, мы можем передать валидный указатель для подписки, и ``nullptr`` для отписки.
    Во втором случае, нам пригодится возвращаемое значение – это количество "событий", которые успели произойти (или точно произойдут в будущем: ``emit`` успел установить бит сигнального состояния, но ещё не успел вызвать ``notify``) прежде, чем мы окончательно отписались. Если наш подписчик состоит из семафора (реализация по-умолчанию), то нам необходимо дождаться всех событий до того, как разрушить его.
* ``reset`` – переводит событие в несигнальное состояние:
    ```c++
    std::size_t Event::reset() noexcept
    {
        if (signal.fetch_and(~bit) & bit) {
            return 1;
        }
        return 0;
    }
    ```
    Метод сбрасывает флаг события, аналогично методу ``subscribe``, возвращает количество событий, которые успели произойти (или точно произойдут в будущем).

Все методы атомарные и неблокирующие. Метод ``emit`` вызовет ``notify``, который, в свою очередь, задействует семафор.
Важное свойство объекта событие – это то, что повторные вызовы ``emit`` только читают атомарную переменную и ничего не делают до тех пор, пока событие вновь не будет сброшено.

Класс ``Handler`` – это простая реализация на основе семафора:
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
Тут ничего сложного. Единственный нюанс – метод ``wait`` принимает некоторый счётчик. А именно, количество ожидаемых событий, т.е. сумма того, что возвращают методы ``subscribe`` и ``reset``.
Обосную необходимость счётчика. Когда события переходят в сигнальное состояние, "подписчик" не всегда вызывает ``wait``, так в семафоре накапливается свой внутренний счётчик. В момент удаления объекта подписчика, он, конечно же, отписывается от объекта события. Но при этом, метод ``emit`` может всё ещё пытаться сделать ``notify``. Теперь-то и пригождается счётчик событий. Перед разрушением "подписчик" ожидает все накопленные в семафоре события. Так мы обеспечиваем полную безопасность метода ``emit`` и разрушение экземпляра класса ``Handler``.

### Подведем итог
Объект события работает в паре с любым объектом, который может изменять свое состояние, и об этом надо уведомить другой поток. Пока всё выглядит также, как работа с парой ``std::mutex`` и ``std::condition_variable``.

Общий алгоритм работы с событием со стороны потока-писателя:
1) Изменить состояние (в нашем случае, канал);
2) Вызвать ``emit``.
Важное отличие от ``std::condition_variable`` – для изменения состояния не требуется захватывать ``std::mutex``.

Общий алгоритм работы с событием со стороны потока-читателя выглядит так:
1) Создать объект ``Handler``;
2) Подписаться на событие;
3) Узнать, произошло ли интересующее нас изменение:
   1) Опросить объект (канал) на предмет интересующих нас изменений;
   2) Если изменений нет, вызвать ``reset``;
   3) Ещё раз опросить объект;
   4) Если изменений нет, то вызвать ``wait``;
   5) Вызвать ``reset``;
   6) Перейти на шаг 3.
4) Отписаться от события.
Пункт 3, можно повторять сколько угодно раз. Объект ``Handler`` можно переиспользовать, поэтому к пункту 4 мы переходим тогда, когда выполнили всю работу.

Пункт 4 сложнее: недостаточно просто вызвать ``subscribe(nullptr)``, необходимо учесть количество произошедших событий, для которых не был вызван ``wait``. Напишем класс, упрощающий работу с объектами событий и подписчиками. Он дополнительно защитит нас от ошибки "забыть отписаться":

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
Мы обернули метод ``wait`` у класса ``Handler`` и метод ``reset`` у класса ``Event``. Класс ``Subscriber`` реализует идиому RAII и инкапсулирует работу со счётчиком произошедших событий.

## Мультиплексирование событий
Как нам теперь объединить несколько объектов событий в один?
Довольно просто, но нужно оговориться, что такой тип события переходит в сигнальное состояние тогда, когда хотя бы одно подсобытие переходит в сигнальное состояние.

Класс мультиплексирования событий:
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

Все методы класса ``EventMux`` мы делегируем подконтрольным событиям. Метод ``poll``, для возвращаемого результата, использует оператор логического "или". В случае ``reset`` и ``subscribe`` суммируем возвращаемые значения.
Единственное отличие – это отсутствие метода ``emit``, для класса ``EventMux`` он не нужен.

## Каналы
Каналы могут быть для чтения или для записи, либо и то, и другое. Интерфейсы у них очень похожи, поэтому я использую префиксы "r" (recv) и "s" (send) для схожих методов.
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
* ``rpoll`` – опросить канал на предмет новых сообщений. ``true`` – если канал готов к чтению, ``false`` – если в канале ничего нет.
* ``urecv`` – прочитать (или получить) следующее сообщение. Суффикс "u" – обозначает "unsafe", этот метод нельзя вызывать, если предварительно не был вызван ``rpoll``, который вернул ``true``.
* ``close`` – закрыть канал. Сообщения, которые ещё остались в канале, останутся доступными для чтения. После вызова этого метода, ``rpoll`` всё ещё может возвращать ``true``, если что-то осталось непрочитанным.
* ``closed`` – думаю, тут все понятно.
* ``revent`` – возвращает ссылку на связанный объект события.
В дополнении к этим методам, интерфейс должен предоставить два типа:
* ``Type`` – тип сообщения. Метод ``urecv`` должен возвращать объект именно этого типа.
* ``REvent`` – тип привязанного объекта события. Метод ``revent`` должен возвращать ссылку на этот тип.

В интерфейсе не используются виртуальные функции, потому что в данной реализации применяется (в основном) статический полиморфизм. Если есть статический полиморфизм, то вариант с виртуальными функциями ничуть не сложно реализовать.

Интерфейс канала для записи, он почти ничем не отличается от ``IChannelInterface``:
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

Оба интерфейса достаточно "низкоуровневые" для прямого использования. Просто так ``usend/urecv`` не вызвать, нужно убедиться, что ``spoll/rpoll`` вернул ``true``, а если нет, то надо работать с объектом события, ссылку на который возвращает ``sevent/revent``. Обычно, мы просто хотим вызвать что-то вроде ``channel.send(value)`` и заблокироваться, если в канале недостаточно места.
Для этого сделаем класс-helper:
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
Класс ``OChannel`` использует ``auto& self = *static_cast<Channel*>(this);`` и наследование от параметра шаблона, а сам параметр, называется ``Channel``. Всё верно, это CRTP.
Пример использования:
```c++
template<class T>
class OChannelInterface: public OChannel<OChannelInterface<T>>
{
public:
    ...
};
```
Готово, теперь у нас есть удобный метод ``send``. Для канала-читателя есть аналогичный класс. Заметим, что метод хоть и удобный, но неэффективный, он каждый раз создаёт объекты ``Handler`` и ``Subscriber`` на стеке и использует их одноразово. Работа через итераторы решают эту проблему.

### Range-based for для канала
Для выполнения требования №4 необходимо реализовать классы итераторов. Я решил добавить отдельный класс ``IRange`` и свободную функцию ``irange``. Класс и функция шаблонные и работают с любыми каналами для чтения. Реализация неидеальная, представлю её целиком:
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
1) Класс ```IRange`` хранит объекты ``Handler`` и ``Subscriber``. Это значит, что подписка на событие происходит в самом начале, а отписка – только один раз в конце работы с итераторами.
2) Разыменование итератора должно возвращать ссылку на сообщение, следовательно, сообщение нужно временно где-то сохранить. Используем класс ``std::aligned_storage_t`` для этой цели.
3) Конструктор и деструктор сообщения вызываются только в определённые моменты, нет необходимости в дополнительных переменных или использовать класс ``std::optional``.

### Мультиплексирование каналов
Теперь у нас есть всё необходимое для мультиплексирования каналов. Как было отмечено выше, мультиплексировать можно двумя способами.

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

Класс сохраняет ссылки на объединяемые каналы в список ``std::tuple``. У него есть свой объект события на основе класса ``EventMux`` и счётчик для реализации ``round-robin``.
Возвращаемый тип – ``std::variant<typename Channels::Type...>``. Все каналы могут иметь разные типы сообщений. Если типы повторяются, то они всё равно будут представлены в ``std::variant`` под своим индексом.
Простая реализация методов:
* ``close`` – вызывает ``close`` для каждого подканала.
* ``closed`` – вызывает ``closed`` для каждого подканала и возвращает ``true``, если все из них вернули ``true``.
* ``rpoll`` – вызывает ``rpoll`` для каждого подканала, но запоминает на каком остановился. Вызов ``rpoll`` в цикле использует массив указателей на функции для стирания типа:
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
* ``urecv`` – вызывает ``urecv`` у того канала, у которого функция ``rpoll`` вернула ``true``. Тут тоже используется массив указателей для стирания типа:
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
Класс ``IChannelAny`` соответствует интерфейсу ``IChannelInterface``. Следовательно, его можно мультиплексировать с другими каналами.

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
Почти всё то же самое, что у класса ``IChannelAny``. Вместо счётчика используется поле ``std::bitset`` для каждого канала по биту – небольшая оптимизация, чтобы повторно не опрашивать каналы. Возвращаемый тип – ``std::tuple<typename Channels::Type...>``.
* ``close`` – вызывает ``close`` для каждого подканала. Тут всё так же.
* ``closed`` – вызывает ``closed`` для каждого подканала и возвращает ``true``, если хотя бы один из них вернул ``true``.
* ``rpoll`` – вызывает ``rpoll`` для каждого подканала, результат сохраняет в битовое поле:
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
* ``urecv`` – вызывает ``urecv`` у всех каналов и объединяет результаты в кортеж:
    ```c++
    template<std::size_t ...I>
    Type urecv(std::index_sequence<I...>)
    {
        states.reset();
        return std::make_tuple(std::get<I>(channels).urecv()...);
    }
    ```
Класс ``IChannelAll`` соответствует интерфейсу ``IChannelInterface``, а это значит, что его тоже можно мультиплексировать с другими каналами.

Используя ``structured binding``, чтение из ``IChannelAll`` можно организовать следующим образом:
```c++
lib::IChannelAll channels_ab(channel_a, channel_b);
for(const auto& [a, b]: lib::irange(channels_ab)) {
    ...
}
```

# Итоги

Каналы (и события) реализуют паттерн компоновщика: мультиплексированные каналы, в свою очередь, тоже могут быть мультиплексированы и так далее, всё будет работать с любым "уровнем вложенности". Все требования к реализации удовлетворены: никакой динамической памяти, никаких дополнительных, сервисных (фоновых) или промежуточных потоков, корутин и т.д. Мы можем обернуть lock-free очередь в канал или сделать тонкую обёртку над функцией-генератором.

Архитектурные возможности библиотеки позволяют реализовать каналы и их мультиплексирование на любой вкус.
Приведу два примера.

## AggregateChannel
Предположим, нам нужно объединить несколько каналов по схеме "или", но все каналы передают один тип сообщения или разные типы сообщений, но все они могут быть преобразованы в один общий тип. При этом, "читателю" не важно из какого именно канала пришло сообщение. Можно использовать обычный ``IChannelAny``, но тогда итоговым типом будет ``std::variant``. Для такого объединения можно написать отдельный класс:
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
Логика работы этого класса практически идентична классу ``IChannelAny``, разница лишь в создаваемом типе сообщения.

## BroadCastChannel
Канал для записи, который рассылает копии сообщений в другие каналы.
Реализация тривиальна:
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
По-умолчанию входной тип вычисляется так ``std::common_type_t<typename Channels::Type ...>``, но можно указать любой другой, главное, чтобы он мог преобразовываться в типы ``typename Channels::Type``.

# Возможное дальнейшее развитие библиотеки

## События ОС

Основа реализации – это класс события и подписчика. Событие может перейти в сигнальное состояние только через вызов метода ``emit``. Данное ограничение не позволяет нам обобщить такие сущности, как файловые дескрипторы или сокеты. Предположим, мы хотим обернуть взаимодействие по сети в абстракцию канала. Создавать сервисный или промежуточный поток – неприемлемо. Что можно с этим сделать в будущем:
* Добавить новый тип сообщений – тонкая обёртка над объектом ОС. У такого класса не будет метода ``emit``;
* Сделать частичную специализацию шаблона класса мультиплексирования событий, если в списке типов событий присутствует "событие ОС";
* Подписчик меняет свой тип или адаптивно подстраивается под общий тип события;
* Объединяем несколько событий ОС для использования механизма мультиплексирования из ОС (например, под системный вызов poll в linux);
* "Обычные" события тоже объединяются, к примеру, в [eventfd](https://man7.org/linux/man-pages/man2/eventfd.2.html) со своим дескриптором. Метод ``emit`` у таких событий будет пробуждать поток через ``eventfd``.
Пока общую картину сложно обрисовать, но интуиция подсказывает, что это более, чем возможно. Дополнительные сложности возникают с каналами с динамическим полиморфизмом, необходимо решить, какой тип события будут возвращать методы ``revent/sevent``.

## Корутины C++20

Мы можем реализовать классы каналов, преобразующие один тип сообщения в другой, через пользовательскую функцию. Реализация тривиальна, но это работает, если преобразование происходит один к одному. Но иногда, возникает потребность преобразования многие к одному (или один во многие). Например, из некоторого канала приходят буферы байтов (``std::span<std::byte>`` или ``std::string_view``), мы можем написать функцию, которая парсит такие данные и возвращает другой объект, например, json-структуру. Хорошо, если у нас такой парсер поддерживает потоковый интерфейс ( [boost::json::stream_parser](https://www.boost.org/doc/libs/1_84_0/libs/json/doc/html/json/ref/boost__json__stream_parser.html)).
В ином случае, нам потребуется промежуточный поток (или ``stackfull`` корутина), задача которого, заключается в одном: парсить множество объектов в один и передавать дальше.

В C++20 есть базовая поддержка ``stackless`` корутин. Интеграция в библиотеку позволит реализовывать сложные преобразователи с минимальными накладными расходами. Генераторы (требование №5) станут более органичными, одна C++ корутина может описать бесконечные и конечные генераторы, и всё это можно завернуть в интерфейс канала. При этом, не нужен пул потоков для выполнения корутин, все они могут выполняться в рамках одного потока, который читает или пишет в канал.

## Output итераторы и мультиплексирование каналов разной направленности

На данный момент каналы для записи не поддерживают итераторы. Мы можем их мультиплексировать, но не смешивать с каналами для чтения. По крайне мере, пока такой функционал не разработан. Идея в том, чтобы появилась возможность написать так:
```c++
    for (auto& [input, output]: lib::IChannelAll{channel_in, channel_out}) {
        ouput = proccess(input);
    }
```
Таким образом, тело цикла будет выполняться только тогда, когда оба канала готовы: во входящем канале есть новое сообщение, а в выходящем – место для отправки.

## Динамическое мультиплексирование

Мы можем мультиплексировать сколь угодно много каналов в один, но менять количество каналов динамически нельзя. Это исправляется добавлением отдельного класса мультиплексора событий и каналов.
Такой канал принимает на вход контейнер (например, ``std::vector`` или ``std::span``) с объединяемыми каналами.
Этот класс сможет работать только с каналами с одним типом сообщений, и объединяемые каналы должны быть с динамическим полиморфизмом.

## Оптимизация вызовов rpoll/spoll

Перед каждым вызовом метода ``usend/urecv`` мы должны убедиться, что канал доступен, вызвав ``rpoll/spoll``.
Можно уменьшить количество вызовов ``rpoll/spoll``, если они вместо ``bool`` будут возвращать количество доступных сообщений для чтения/записи. Это количество необходимо где-то хранить, так мы выиграем в производительности, немного увеличив потребление памяти.
