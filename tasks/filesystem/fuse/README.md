# Содержание задания

Задание заключается в написании эмулятора вычислительного процессора в виде драйвера на базе `libfuse` версии 3.2+.
Требования к драйверу:

1. структура директорий драйвера:

    ```
    /dev/
    └── mycpu/
        ├── unit0
        │   ├── pram
        │   └── lram
        ├── unit1
        │   ├── pram
        │   └── lram
        ...
        ├── unitn
        │   ├── pram
        │   └── lram
        └── ctrl
    ```
1. количество устройств `unit` необходимо задавать диначески при запуске драйвера, т.е. во время
исполнения количество юнитов не меняется;
1. запуск и остановка устройства `unit` осуществляется с помощью устройства `ctrl`:

    ```
    $ echo 3 > /dev/mycpu/ctrl
    $ cat /dev/mycpu/ctrl
    3
    ```
1. примерный сценарий использования:

    ```
    $ cat testdata.bin > /dev/mycpu/unit0/lram
    $ cat testdata.bin > /dev/mycpu/unit1/lram
    $ cat | tee /dev/unit0/pram /dev/unit1/pram
    #include <algorithm>
    
    int entrypoint(uint32_t size, uint8_t* ram)
    {
        std::sort(ram, ram + size);
        return 0;
    }
    <Ctrl-D>
    $ echo 0 > /dev/mycpu/ctrl
    $ echo 1 > /dev/mycpu/ctrl
    $ cat /dev/mycpu/ctrl
    1
    0
    $ diff /dev/mycpu/unit0/lram /dev/mycpu/unit1/lram
    $ diff reference.bin /dev/mycpu/unit0/lram
    ```
1. в проекте должно быть поддержано юнит-тестирование через `Catch2`;
1. поддержана документация проекта через `doxygen`;
1. поддержана генерация установчного пакета;
1. сборка проекта осуществляется через `CMake`.

Ведение проекта осуществляется поэтапно, т.е. не все выше описанные требования должны сразу выполняться.