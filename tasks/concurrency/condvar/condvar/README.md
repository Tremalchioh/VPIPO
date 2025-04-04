# Условная переменная (кондвар)

Условные переменные (_condition variables_) или просто _кондвары_ (_condvars_) – примитив синхронизации, который предоставляет потокам функциональность 1) блокирующего ожидания событий и 2) уведомления об их реализации. 

Под событием в контексте условных переменных понимается модификация разделяемого состояния, защищенного мьютексом (например, очереди задач в пуле потоков).

В этой задаче вы должны реализовать условную переменную.

## Операции

У условной переменной есть один метод для ожидания – `Wait`, и два метода для нотификации – `NotifyOne` и `NotifyAll`.

Метод `Wait` можно вызывать только внутри критической секции, в него передается захваченный мьютекс.

Семантика `cv.Wait(mutex)` –

1. *Aтомарно* 1) отпустить `mutex` и 2) встать в очередь ожидания нотификации от `cv.NotifyOne()` или `cv.NotifyAll()`.
2. После пробуждения захватить обратно отпущенный `mutex` и завершить вызов.

До начала и после завершения вызова `cv.Wait(mutex)` поток владеет мьютексом.

Под атомарностью метода `Wait` следует понимать вот что: не допускается сценарий, когда поток уже отпустил мьютекс, но еще не встал в очередь ожидания условной переменной, а в этот момент другой поток вызвал `cv.NotifyOne()` и никого не разбудил.

Метод `Wait` допускает *ложные пробуждения* (*spurious wakeups*):
- Вызов `cv.Wait(mutex)` может вернуть управление даже без нотификации от других потоков
- После вызова `NotifyOne` могут проснуться несколько потоков

Ваша реализация наверняка будет подвержена подобным проблемам, обратите на это внимание!

Но оказывается, что корректные паттерны использования условных переменных сохраняют свою корректность даже в подобных ситуациях!

Семантика `cv.NotifyOne()` / `cv.NotifyAll()` – разбудить один из потоков / все потоки, спящие в очереди ожидания в вызове `Wait`.

Условная переменная не запоминает нотификации: если в момент вызова `NotifyOne` или `NotifyAll` ни один поток не ждал внутри вызова `Wait`, то нотификация пролетит без какого-либо эффекта.

Изучите:
* [`std::condition_variable`](https://en.cppreference.com/w/cpp/thread/condition_variable)

## Отношения с другими примитивами

В промышленном коде кондвары не следует использовать напрямую. Это достаточно низкоуровневый инструмент, с помощью которого можно строить примитивы для конкретных сценариев синхронизации: [барьеры](https://en.cppreference.com/w/cpp/thread/barrier), [семафоры](https://en.cppreference.com/w/cpp/thread/counting_semaphore), [латчи](https://en.cppreference.com/w/cpp/thread/latch) и т.п.

Каждый такой примитив фиксирует конкретные условия для ожидания и пробуждения:

- Поток, последним подошедший к барьеру, будит остальных участников, которые подошли к барьеру раньше
- С помощью латчей одни потоки дожидаются завершения работы других (вспомните `Join` в пуле потоков)
 
## Реализация

Как и мьютекс, кондвар должен блокировать и будить ждущие потоки. А значит при реализации нам потребуется уже знакомый инструмент – `futex`, с которым мы работаем через операции `wait` / `notify_{one,all}` у `atomic`.

## Фьютекс и кондвар

Можно сказать, что в некотором смысле кондвар обобщает фьютекс: ждать можно на сколь угодно сложном состоянии / предикате.

## A-B-A

Скорее всего ваша реализация будет подвержена [_A-B-A problem_](https://en.wikipedia.org/wiki/ABA_problem) из-за переполнения 32-битного счетчика.

## Кондвары в библиотеке NPTL

См. [`pthread_cond_wait`](https://github.com/lattera/glibc/blob/895ef79e04a953cac1493863bcae29ad85657ee1/nptl/pthread_cond_wait.c#L193)

Эта реализация намного сложнее, но зато не подвержена A-B-A.