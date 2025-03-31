
#include <iostream>
#include <fstream>
#include <random>
#include <vector>
#include <string>
#include <algorithm>
#include <queue>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

// Функция для получения размера файла
static size_t getFileSize(const std::string &filename) {
  struct stat st;
  if (stat(filename.c_str(), &st) != 0) {  // Получаем информацию о файле
    perror("stat failed");
    exit(1);
  }
  return (size_t)st.st_size;  // Возвращаем размер файла в байтах
}

// Структура для хранения информации о mmap-области
struct MappedRegion {
  void* ptr;          // Указатель на начало данных
  size_t mappingSize; // Полный размер отображения (включая выравнивание)
  void* mmappedAddr;  // Адрес, возвращенный mmap (до выравнивания)
};

// Функция для создания выровненного mmap отображения
static MappedRegion mmapWithPageAlign(size_t offsetInFile, size_t mapSizeBytes,
                                      int protFlags, int mapFlags, int fd) {
  MappedRegion result;
  result.ptr = nullptr;
  result.mappingSize = 0;
  result.mmappedAddr = nullptr;

  if (mapSizeBytes == 0) return result;  // Нулевой размер - ничего не делаем

  // Получаем размер страницы памяти
  long ps = sysconf(_SC_PAGE_SIZE);
  if (ps < 1) ps = 4096;  // Если не удалось получить, используем 4K по умолчанию

  // Вычисляем выровненное смещение
  off_t alignOffset = (offsetInFile / ps) * ps;
  size_t diff = offsetInFile - alignOffset;  // Разница между реальным и выровненным смещением
  size_t fullSize = mapSizeBytes + diff;    // Полный размер с учетом выравнивания

  // Создаем mmap отображение
  void* addr = mmap(nullptr, fullSize, protFlags, mapFlags, fd, alignOffset);
  if (addr == MAP_FAILED) return result;  // В случае ошибки

  // Заполняем структуру результата
  result.mmappedAddr = addr;
  result.mappingSize = fullSize;
  result.ptr = static_cast<char*>(addr) + diff;  // Корректируем указатель на реальные данные

  return result;
}

// Функция генерации тестового файла со случайными числами
static void generateFile(const std::string &filename, size_t count, bool sorted) {
  std::random_device rd;  // Источник случайных чисел
  std::mt19937_64 gen(rd());  // Генератор случайных чисел
  // Равномерное распределение для всех возможных 64-битных целых
  std::uniform_int_distribution<long long> dist(
      std::numeric_limits<long long>::min(),
      std::numeric_limits<long long>::max()
  );

  // Открываем файл для записи в бинарном режиме
  std::ofstream ofs(filename, std::ios::binary);
  if (!ofs) {
    std::cerr << "Cannot open file for writing: " << filename << "\n";
    exit(1);
  }

  // Генерируем данные
  std::vector<long long> data(count);
  for (size_t i = 0; i < count; i++) data[i] = dist(gen);

  // Сортируем если нужно
  if (sorted) std::sort(data.begin(), data.end());

  // Записываем данные в файл
  ofs.write(reinterpret_cast<char*>(data.data()), count*sizeof(long long));
  ofs.close();

  std::cout << "Generated " << count << " 64-bit integers into " << filename
            << (sorted ? " (sorted)." : ".") << "\n";
}

// Функция проверки отсортированности файла
static void checkFile(const std::string &filename) {
  size_t fs = getFileSize(filename);
  // Проверяем что размер файла кратен 8 байтам (размеру long long)
  if (fs % sizeof(long long) != 0) {
    std::cerr << "File size not multiple of 8 bytes => not 64-bit ints.\n";
    exit(1);
  }

  size_t total = fs / sizeof(long long);  // Количество элементов
  if (total < 2) {
    std::cout << "File has 0 or 1 element => trivially sorted.\n";
    return;
  }

  // Открываем файл для чтения
  std::ifstream ifs(filename, std::ios::binary);
  if (!ifs) {
    std::cerr << "Cannot open file for reading: " << filename << "\n";
    exit(1);
  }

  const size_t BUF = 1 << 15;  // Размер буфера для чтения (32K элементов)
  std::vector<long long> buf(BUF);
  long long prev;
  bool first = true;
  size_t readCount = 0;

  // Читаем файл по частям
  while (readCount < total) {
    size_t toRead = std::min(BUF, total - readCount);
    ifs.read(reinterpret_cast<char*>(buf.data()), toRead*sizeof(long long));

    if (!ifs) {
      std::cerr << "Error reading file.\n";
      exit(1);
    }

    // Проверяем порядок элементов в буфере
    for (size_t i = 0; i < toRead; i++) {
      if (!first) {
        if (buf[i] < prev) {  // Нарушен порядок сортировки
          std::cerr << "File is NOT sorted (found " << buf[i]
                    << " after " << prev << ").\n";
          return;
        }
      }
      prev = buf[i];
      first = false;
    }
    readCount += toRead;
  }

  std::cout << "File is sorted ascending.\n";
}

// Функция для сортировки части файла (чанка)
static void sortChunkAndWrite(int inFD, int outFD, size_t offElems, size_t count,
                              size_t inOffBytes, size_t outOffBytes, size_t mapSizeBytes,
                              bool inPlace) {
  // Создаем mmap отображение для входных данных
  auto region = mmapWithPageAlign(inOffBytes, mapSizeBytes, PROT_READ|PROT_WRITE, MAP_SHARED, inFD);
  if (region.mmappedAddr == MAP_FAILED && mapSizeBytes > 0) {
    perror("sortChunk: mmap failed");
    exit(1);
  }
  if (!region.ptr && mapSizeBytes > 0) {
    std::cerr << "sortChunk: got null region\n";
    exit(1);
  }

  // Сортируем данные в памяти
  long long* ptr = (long long*)region.ptr;
  std::sort(ptr, ptr + count);

  if (!inPlace) {
    // Если сортировка не на месте, создаем mmap для выходных данных
    auto outRegion = mmapWithPageAlign(outOffBytes, mapSizeBytes, PROT_READ|PROT_WRITE, MAP_SHARED, outFD);
    if (outRegion.mmappedAddr == MAP_FAILED && mapSizeBytes > 0) {
      perror("sortChunk: mmap out failed");
      if (region.mmappedAddr != MAP_FAILED && region.mappingSize > 0) {
        munmap(region.mmappedAddr, region.mappingSize);
      }
      exit(1);
    }

    // Копируем отсортированные данные в выходной файл
    if (outRegion.ptr && mapSizeBytes > 0) {
      std::memcpy(outRegion.ptr, region.ptr, mapSizeBytes);
    }

    // Освобождаем ресурсы
    if (outRegion.mmappedAddr != MAP_FAILED && outRegion.mappingSize > 0) {
      munmap(outRegion.mmappedAddr, outRegion.mappingSize);
    }
  }

  // Освобождаем ресурсы входного отображения
  if (region.mmappedAddr != MAP_FAILED && region.mappingSize > 0) {
    munmap(region.mmappedAddr, region.mappingSize);
  }
}

// Структура для хранения информации о "ранне" (отсортированной последовательности)
struct RunInfo {
  size_t offset;  // Смещение в файле (в элементах)
  size_t length;  // Длина (в элементах)
};

// Функция для создания начальных отсортированных последовательностей (раннов)
static std::vector<RunInfo> createInitialRuns(int inFD, int outFD, size_t totalElems, size_t chunkBytes) {
  std::vector<RunInfo> runs;
  size_t off = 0;
  size_t chunkElems = chunkBytes / sizeof(long long);
  if (chunkElems < 1) chunkElems = 1;  // Минимум 1 элемент

  // Разбиваем файл на чанки и сортируем каждый
  while (off < totalElems) {
    size_t c = std::min(chunkElems, totalElems - off);  // Размер текущего чанка
    size_t inOffB = off * sizeof(long long);            // Смещение в байтах
    size_t mapSize = c * sizeof(long long);              // Размер отображения

    // Сортируем чанк и записываем
    sortChunkAndWrite(inFD, outFD, off, c, inOffB, inOffB, mapSize, false);

    // Сохраняем информацию о ранне
    RunInfo r;
    r.offset = off;
    r.length = c;
    runs.push_back(r);

    off += c;  // Переходим к следующему чанку
  }

  return runs;
}

// Структура для элемента в min-куче (используется при слиянии)
struct HeapItem {
  long long value;  // Значение элемента
  int runIdx;       // Индекс ранна, из которого взят элемент
};

// Оператор сравнения для кучи (min-куча)
static bool operator>(const HeapItem &a, const HeapItem &b) {
  return a.value > b.value;
}

// Состояние одного ранна при слиянии
struct MergeRunState {
  size_t fileOffset;  // Смещение в файле
  size_t length;      // Полная длина ранна
  size_t consumed;    // Сколько элементов уже обработано
  std::vector<long long> buffer;  // Буфер для чтения
  size_t bufPos;      // Текущая позиция в буфере
  bool done;          // Флаг завершения
};

// Функция для многопутевого слияния раннов
static void multiWayMerge(int inFD, int outFD, const std::vector<RunInfo> &runs,
                          size_t outOffset, size_t memBytes) {
  if (runs.empty()) return;

  // Вычисляем общее количество элементов
  size_t totalLen = 0;
  for (auto &r : runs) totalLen += r.length;

  size_t k = runs.size();  // Количество раннов для слияния

  // Специальная обработка случая с одним ранном (просто копируем)
  if (k == 1) {
    size_t off = runs[0].offset;
    size_t ln = runs[0].length;
    size_t totalB = ln * sizeof(long long);
    size_t stepB = memBytes > 0 ? memBytes : (1 << 20);  // 1MB шаг по умолчанию

    size_t done = 0;
    while (done < totalB) {
      size_t s = totalB - done;
      if (s > stepB) s = stepB;

      // Отображаем часть входного файла
      auto inMap = mmapWithPageAlign(off * sizeof(long long) + done, s, PROT_READ, MAP_SHARED, inFD);
      if (inMap.mmappedAddr == MAP_FAILED && s > 0) {
        perror("multiWayMerge single-run inMap fail");
        exit(1);
      }

      // Отображаем часть выходного файла
      auto outMap = mmapWithPageAlign(outOffset * sizeof(long long) + done, s,
                                      PROT_READ|PROT_WRITE, MAP_SHARED, outFD);
      if (outMap.mmappedAddr == MAP_FAILED && s > 0) {
        perror("multiWayMerge single-run outMap fail");
        if (inMap.mmappedAddr != MAP_FAILED && inMap.mappingSize > 0) {
          munmap(inMap.mmappedAddr, inMap.mappingSize);
        }
        exit(1);
      }

      // Копируем данные
      if (inMap.ptr && outMap.ptr && s > 0) {
        std::memcpy(outMap.ptr, inMap.ptr, s);
      }

      // Освобождаем ресурсы
      if (inMap.mmappedAddr != MAP_FAILED && inMap.mappingSize > 0) {
        munmap(inMap.mmappedAddr, inMap.mappingSize);
      }
      if (outMap.mmappedAddr != MAP_FAILED && outMap.mappingSize > 0) {
        munmap(outMap.mmappedAddr, outMap.mappingSize);
      }

      done += s;
    }
    return;
  }

  // Вычисляем сколько памяти выделить каждому ранну
  size_t memForEach = memBytes / (k + 1);  // +1 для выходного буфера
  memForEach = (memForEach / 8) * 8;       // Выравниваем на 8 байт
  if (memForEach < 8) memForEach = 8;      // Минимум 8 байт

  // Инициализируем состояние для каждого ранна
  std::vector<MergeRunState> st(k);
  size_t outBufBytes = memForEach;                  // Размер выходного буфера
  size_t outBufCount = outBufBytes / sizeof(long long);
  if (outBufCount < 1) outBufCount = 1;
  std::vector<long long> outBuf(outBufCount);       // Выходной буфер
  size_t outPos = 0;                                // Позиция в выходном буфере
  size_t curOut = outOffset;                        // Текущая позиция в выходном файле

  for (size_t i = 0; i < k; i++) {
    st[i].fileOffset = runs[i].offset;
    st[i].length = runs[i].length;
    st[i].consumed = 0;
    st[i].done = false;
    st[i].bufPos = 0;

    // Выделяем буфер для этого ранна
    size_t eachCount = memForEach / sizeof(long long);
    if (eachCount < 1) eachCount = 1;
    st[i].buffer.resize(eachCount);
  }

  // Создаем min-кучу для слияния
  std::priority_queue<HeapItem, std::vector<HeapItem>, std::greater<HeapItem>> minHeap;

  // Лямбда-функция для заполнения буфера ранна
  auto refill = [&](int idx) {
    MergeRunState &rs = st[idx];
    rs.bufPos = 0;
    size_t left = rs.length - rs.consumed;
    if (left == 0) { rs.done = true; return; }  // Ранн закончился

    size_t space = rs.buffer.size();
    if (space > left) space = left;

    // Отображаем часть файла в память
    size_t startB = (rs.fileOffset + rs.consumed) * sizeof(long long);
    size_t mapB = space * sizeof(long long);
    auto inMap = mmapWithPageAlign(startB, mapB, PROT_READ, MAP_SHARED, inFD);

    if (inMap.mmappedAddr == MAP_FAILED && mapB > 0) {
      perror("multiWayMerge: refill mmap failed");
      exit(1);
    }

    // Копируем данные в буфер
    if (inMap.ptr && mapB > 0) {
      std::memcpy(rs.buffer.data(), inMap.ptr, mapB);
    }

    // Освобождаем mmap
    if (inMap.mmappedAddr != MAP_FAILED && inMap.mappingSize > 0) {
      munmap(inMap.mmappedAddr, inMap.mappingSize);
    }

    rs.consumed += space;
  };

  // Первоначальное заполнение буферов и кучи
  for (size_t i = 0; i < k; i++) {
    refill(i);
    if (!st[i].done) {
      HeapItem itm;
      itm.value = st[i].buffer[0];
      itm.runIdx = (int)i;
      minHeap.push(itm);
      st[i].bufPos = 1;
    }
  }

  // Лямбда-функция для записи выходного буфера в файл
  auto flushOut = [&]() {
    if (outPos == 0) return;

    size_t mapB = outPos * sizeof(long long);
    size_t startB = curOut * sizeof(long long);

    // Отображаем часть выходного файла
    auto outMap = mmapWithPageAlign(startB, mapB, PROT_READ|PROT_WRITE, MAP_SHARED, outFD);
    if (outMap.mmappedAddr == MAP_FAILED && mapB > 0) {
      perror("multiWayMerge: flushOutBuffer mmap failed");
      exit(1);
    }

    // Копируем данные
    if (outMap.ptr && mapB > 0) {
      std::memcpy(outMap.ptr, outBuf.data(), mapB);
    }

    // Освобождаем ресурсы
    if (outMap.mmappedAddr != MAP_FAILED && outMap.mappingSize > 0) {
      munmap(outMap.mmappedAddr, outMap.mappingSize);
    }

    curOut += outPos;
    outPos = 0;
  };

  // Основной цикл слияния
  while (!minHeap.empty()) {
    // Извлекаем минимальный элемент
    auto top = minHeap.top();
    minHeap.pop();

    // Добавляем в выходной буфер
    outBuf[outPos++] = top.value;
    if (outPos == outBufCount) flushOut();  // Буфер заполнен - записываем

    int ridx = top.runIdx;
    MergeRunState &rs = st[ridx];

    if (!rs.done) {
      // Если буфер текущего ранна закончился - заполняем снова
      if (rs.bufPos >= rs.buffer.size()) {
        refill(ridx);
        rs.bufPos = 0;
        if (rs.done) continue;  // Ранн закончился
      }

      // Добавляем следующий элемент из этого ранна в кучу
      HeapItem nxt;
      nxt.value = rs.buffer[rs.bufPos];
      nxt.runIdx = ridx;
      minHeap.push(nxt);
      rs.bufPos++;
    }
  }

  flushOut();  // Записываем оставшиеся данные
}

// Функция для выполнения одного прохода многопутевого слияния
static std::vector<RunInfo> multiWayMergePass(int inFD, int outFD, const std::vector<RunInfo> &runs,
                                              size_t memBytes, size_t maxK) {
  std::vector<RunInfo> newRuns;
  newRuns.reserve((runs.size() + maxK - 1) / maxK);  // Резервируем память

  size_t outOff = 0;
  // Разбиваем ранны на группы по maxK и сливаем каждую группу
  for (size_t i = 0; i < runs.size(); i += maxK) {
    size_t grp = std::min(maxK, runs.size() - i);
    std::vector<RunInfo> group;
    group.reserve(grp);

    size_t totalLen = 0;
    for (size_t j = 0; j < grp; j++) {
      group.push_back(runs[i + j]);
      totalLen += runs[i + j].length;
    }

    // Сливаем группу раннов
    multiWayMerge(inFD, outFD, group, outOff, memBytes);

    // Добавляем информацию о новом ранне
    RunInfo nr;
    nr.offset = outOff;
    nr.length = totalLen;
    newRuns.push_back(nr);

    outOff += totalLen;
  }

  return newRuns;
}

// Основная функция внешней сортировки
static void externalSort(const std::string &filename, size_t memoryLimitMB) {
  // Открываем входной файл
  int fd = open(filename.c_str(), O_RDWR);
  if (fd < 0) {
    perror("open input file");
    exit(1);
  }

  // Получаем размер файла и проверяем его
  size_t fs = getFileSize(filename);
  if (fs % sizeof(long long) != 0) {
    std::cerr << "File size not multiple of 8 => not 64-bit ints.\n";
    close(fd);
    exit(1);
  }

  size_t total = fs / sizeof(long long);  // Количество элементов
  if (total <= 1) {
    std::cout << "File has <= 1 element, already sorted.\n";
    close(fd);
    return;
  }

  // Проверяем, не отсортирован ли файл уже
  {
    std::cout << "Checking if the file is already sorted...\n";
    std::ifstream f(filename, std::ios::binary);
    if (f) {
      const size_t SAMP = 1000;  // Количество проверяемых точек
      bool sorted = true;
      long long prev = 0, cur = 0;
      size_t step = total / SAMP;
      if (step < 1) step = 1;

      bool first = true;
      size_t idx = 0;
      while (idx < total) {
        f.seekg(idx * sizeof(long long), std::ios::beg);
        if (!f.read(reinterpret_cast<char*>(&cur), sizeof(cur))) break;

        if (!first) {
          if (cur < prev) {  // Нарушен порядок
            sorted = false;
            break;
          }
        }

        first = false;
        prev = cur;
        idx += step;
      }

      f.close();
      if (sorted) {
        std::cout << "✅ File is already sorted. Skipping sorting.\n";
        close(fd);
        return;
      }
    }
  }

  // Определяем объем используемой памяти
  size_t usedMem = 0;
  if (memoryLimitMB == 0) {
    // Если лимит не задан, используем 10% от размера файла
    usedMem = fs / 10;
    long ps = sysconf(_SC_PAGE_SIZE);
    if (ps < 1) ps = 4096;
    if (usedMem < (size_t)ps) usedMem = ps;  // Не меньше размера страницы
  } else {
    usedMem = memoryLimitMB * 1024ULL * 1024ULL;  // Конвертируем MB в байты
  }

  std::cout << "File has " << total << " elements (" << fs << " bytes). Using up to "
            << (usedMem / (1024.0 * 1024.0)) << " MB of mmap.\n";
  std::cout << "🔹 Starting external multi-way mergesort...\n";

  // Создаем временный файл
  std::string tempName = filename + ".tmp_sort";
  int fdTemp = open(tempName.c_str(), O_RDWR|O_CREAT|O_TRUNC, 0666);
  if (fdTemp < 0) {
    perror("open temp file");
    close(fd);
    exit(1);
  }

  // Устанавливаем размер временного файла
  if (ftruncate(fdTemp, fs) != 0) {
    perror("ftruncate temp");
    close(fdTemp);
    close(fd);
    exit(1);
  }

  // Создаем начальные отсортированные последовательности
  size_t chunkBytes = usedMem;
  if (chunkBytes < 8) chunkBytes = 8;
  auto runs = createInitialRuns(fd, fdTemp, total, chunkBytes);

  bool flip = true;  // Флаг для переключения между файлами
  std::vector<RunInfo> currentRuns = runs;
  int inFD = fdTemp;
  int outFD = fd;

  // Функция для вычисления максимального количества путей слияния (k)
  auto computeMaxK = [&](size_t memB) {
    const size_t minBuf = 16UL * 1024UL;  // Минимальный буфер на ранн - 16KB
    if (memB < minBuf * 2) return (size_t)2;  // Если памяти мало - минимум 2

    size_t g = (memB / (minBuf)) - 1;  // Вычисляем возможное k
    if (g < 2) g = 2;
    if (g > 1024) g = 1024;  // Ограничиваем максимальное k
    return g;
  };

  size_t maxK = computeMaxK(usedMem);

  // Основной цикл слияния, пока не останется один ранн
  while (currentRuns.size() > 1) {
    if (ftruncate(outFD, fs) != 0) {  // Устанавливаем размер выходного файла
      perror("ftruncate outFD");
      close(fd);
      close(fdTemp);
      exit(1);
    }

    // Выполняем проход слияния
    auto newRuns = multiWayMergePass(inFD, outFD, currentRuns, usedMem, maxK);
    currentRuns = newRuns;

    // Меняем файлы местами
    int tmp = inFD;
    inFD = outFD;
    outFD = tmp;
    flip = !flip;
  }

  // Если последняя запись была во временный файл - копируем обратно
  if (!flip) {
    lseek(fdTemp, 0, SEEK_SET);
    lseek(fd, 0, SEEK_SET);

    const size_t BUFSZ = 1 << 20;  // 1MB буфер
    std::vector<char> buf(BUFSZ);
    size_t left = fs;

    // Копируем данные блоками
    while (left > 0) {
      size_t s = (left < BUFSZ ? left : BUFSZ);
      ssize_t rd = read(fdTemp, buf.data(), s);
      if (rd < 0) {
        perror("read fdTemp final copy");
        exit(1);
      }
      if (rd == 0) break;

      ssize_t wr = write(fd, buf.data(), rd);
      if (wr < 0) {
        perror("write final to original");
        exit(1);
      }

      left -= wr;
    }
  }

  // Закрываем файлы и удаляем временный
  close(fd);
  close(fdTemp);
  remove(tempName.c_str());
  std::cout << "Sorting complete.\n";
}

// Главная функция
int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage:\n"
              << argv[0] << " --gen <filename> <count> [sorted]\n"
              << argv[0] << " --check <filename>\n"
              << argv[0] << " <filename> [limitMB]\n";
    return 1;
  }

  // Режим генерации тестового файла
  if (std::string(argv[1]) == "--gen") {
    if (argc < 4) {
      std::cerr << "Usage: " << argv[0] << " --gen <filename> <count> [sorted]\n";
      return 1;
    }

    std::string fn = argv[2];
    size_t cnt = 0;
    try {
      cnt = std::stoull(argv[3]);
    } catch (...) {
      std::cerr << "Invalid count.\n";
      return 1;
    }

    bool mkSorted = false;
    if (argc >= 5 && std::string(argv[4]) == "sorted") mkSorted = true;

    generateFile(fn, cnt, mkSorted);
    return 0;
  }

  // Режим проверки сортировки
  if (std::string(argv[1]) == "--check") {
    if (argc < 3) {
      std::cerr << "Usage: " << argv[0] << " --check <filename>\n";
      return 1;
    }

    checkFile(argv[2]);
    return 0;
  }

  // Режим сортировки
  std::string fn = argv[1];
  size_t limitMB = 0;

  if (argc >= 3) {
    try {
      limitMB = std::stoull(argv[2]);
    } catch (const std::invalid_argument&) {
      std::cerr << "Error: Invalid memory limit. Please enter a valid number.\n";
      return 1;
    } catch (const std::out_of_range&) {
      std::cerr << "Error: Memory limit too large.\n";
      return 1;
    }
  }

  externalSort(fn, limitMB);
  return 0;
}

