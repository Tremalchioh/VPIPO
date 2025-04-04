# Считающий семафор

Семафор – это автомат с жетонами, который поддерживает две операции:

- `Acquire()` – Взять один жетон из автомата. Если автомат пуст, то заблокироваться до появления в нем жетонов.
- `Release()` – Положить (вернуть) в автомат один жетон.
 
Начальное число жетонов в семафоре задается в конструкторе.

Поток, забравший жетон из семафора, получает право доступа к некоторому ограниченному пулу ресурсов.

Будем считать, что в семафор помещается неограниченное количество жетонов.

Семафор никакие жетоны явно не хранит, а просто поддерживает счетчик.

Другая интерпретация:

Семафор – это атомарный счетчик с операциями инкремента и декремента. При попытке декрементировать нулевой счетчик поток блокируется до тех пор, пока другой поток не поднимет значение счетчика выше нуля.

## Семафоры в `std`

См. [std::counting_semaphore](https://en.cppreference.com/w/cpp/thread/counting_semaphore)

## Очередь

Семафор – простой, но при этом выразительный примитив синхронизации. 

- [Semaphores are Surprisingly Versatile](https://preshing.com/20150316/semaphores-are-surprisingly-versatile/)
- [Little book of semaphores](http://greenteapress.com/semaphores/LittleBookOfSemaphores.pdf)

В этой задаче вы должны с помощью семафоров реализовать очередь для передачи данных между потоками.

## Задание

1) Реализуйте считающий семафор с помощью условных переменных.

2) С помощью семафоров реализуйте очередь для передачи данных между потоками.

Шаблон решения – файлы [`semaphore.hpp`](semaphore.hpp) и [`blocking_queue.hpp`](blocking_queue.hpp).

### `BlockingQueue`

При реализации очереди для синхронизации потоков допускается использовать _только_ семафоры. Другие примитивы (атомики, мьютексы, кондвары) использовать нельзя. 

Писать условную логику (`if`, `while`) в очереди тоже нельзя.

## `TaggedSemaphore`

Клаcc `Semaphore` позволяет использовать себя неправильно:  

- Пользователь может позвать `Release` на семафоре даже если перед этим не получил от него токена через `Acquire`, т.е. может генерировать токены из воздуха.
- Можно перекладывать токены между разными семафорами, что не всегда имеет смысл.

Обе эти проблемы решает [`TaggedSemaphore`](tagged_semaphore.hpp):

- Теперь токен имеет явное представление – `Token`. Получить токен можно только из вызова `Acquire`. Копировать токен нельзя, можно только перемещать.
  
- Сам семафор параметризуется типом `Tag`. Если пользователь перекладывает токены между семафорами, то должен пометить эти семафоры общим тегом, в противном случае код не скомпилируется.

В реализации очереди возникает потребность перемещать токены между семафорами, так что для них стоит завести общий тэг.