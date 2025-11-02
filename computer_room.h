#pragma once
#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>

class ComputerRoom {
private:
  std::mutex mtx; 
    std::condition_variable cv; // Условная переменная для ожидания событий

    const int capacity = 20; // Максимальная вместимость компьютерного класса
    const int total_ks40 = 30; // Общее кол-во студентов в КС-40
    const int total_ks44 = 24; // Общее кол-во студентов в КС-44
    const int need_ks40 = 15; // Необходимое кол-во студентов КС-40 для начала занятия
    const int need_ks44 = 12; // Необходимое кол-во студентов КС-44 для начала занятия

    // Текущее состояние компьютерного класса
    int occupancy = 0; // Кол-во студентов в классе
    int present_ks40 = 0; // Кол-во студентов КС-40 в классе
    int present_ks44 = 0; // Кол-во студентов КС-44 в классе
    int current_group = 0; // Группа, занимающая класс (0 - нет, 1 - КС-40, 2 - КС-44)
    bool class_in_session = false; // Флаг, что занятие в процессе

    std::atomic<bool> stop_flag{false}; // Флаг для остановки всех потоков

    // Информация о студентах
    std::vector<int> visits_ks40; // Кол-во посещений для каждого студента КС-40
    std::vector<int> visits_ks44; // Кол-во посещений для каждого студента КС-44
    std::vector<bool> in_room_ks40; // Флаги присутствия студентов КС-40 в классе
    std::vector<bool> in_room_ks44; // Флаги присутствия студентов КС-44 в классе
    std::vector<bool> attended_this_session_ks40; // Флаги посещения текущего занятия для КС-40
    std::vector<bool> attended_this_session_ks44; // Флаги посещения текущего занятия для КС-44

    // Доп методы
    int get_random_time();
    bool can_start_class(int group);
    void start_class_locked(int group);


public:
    /**
     * @brief Конструктор класса ComputerRoom
     * 
     * Инициализирует векторы для хранения информации о студентах и устанавливает начальное состояние компьютерного класса.
     */
    ComputerRoom();
    
    /**
     * @brief Останавливает все потоки студентов
     * 
     * Устанавливает флаг остановки и оповещает все ожидающие потоки.
     */
        void stop() {
        stop_flag = true;
        cv.notify_all();
    }

    
    /**
     * @brief Поведение студента в компьютерном классе
     * 
     * @param group Номер группы (1 - КС-40, 2 - КС-44)
     * @param student_id Идентификатор студента в группе
     * 
     * Метод моделирует поведение студента: попытки войти в класс, ожидание начала занятия, получение посещений и выход из класса.
     */
    void student_behavior(int group, int student_id) {
        // Работает до флага остановки
        while (!stop_flag) {
    
            // Генерируем случайное время ожидания перед попыткой входа, как будто студент решает приходить ли ему на занятие
            int S = get_random_time();
            
            // Блок с захватом мьютекса для проверки условий и изменения состояния
            {
                std::unique_lock<std::mutex> lock(mtx);
                if (stop_flag) return;
    
                // Время ожидания студентом начала занятия не более S секунд
                auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(S);
    
                // Внутренний цикл ожидания возможности войти в класс
                while (true) {
                    if (stop_flag) return;
    
                    /**
                     * @condition Условия для входа в класс: если есть свободные места, занятие не идет, идет занятие группы студента
                     */
                    bool can_enter_now = (occupancy < capacity) && (!class_in_session || current_group == group);
    
                    if (can_enter_now) {
                        // Когла получилось войти в класс, обновляем его заполненность
=                        occupancy++;
                        if (group == 1) { 
                            in_room_ks40[student_id] = true; 
                            present_ks40++; 
                        }
                        else { 
                            in_room_ks44[student_id] = true; 
                            present_ks44++; 
                        }
    
                        std::cout << "Студент " << student_id << " из группы " << (group == 1 ? "КС-40" : "КС-44")
                            << " вошел (occupancy=" << occupancy << ", КС-40=" << present_ks40 << ", КС-44=" << present_ks44 << ")\n";
    
                        // Проверка для начала занятия
                        if (!class_in_session && can_start_class(group)) {
                            start_class_locked(group);
                        }
    
                        // + посещение студенту, если пришел на занятие, даже после начала
                        if (class_in_session && current_group == group && 
                            !((group == 1) ? attended_this_session_ks40[student_id] : attended_this_session_ks44[student_id])) {
                            if (group == 1) { 
                                visits_ks40[student_id]++; 
                                attended_this_session_ks40[student_id] = true; 
                            }
                            else { 
                                visits_ks44[student_id]++; 
                                attended_this_session_ks44[student_id] = true; 
                            }
                            std::cout << "  (засчитано посещение для студента " << student_id << ", всего="
                                << (group == 1 ? visits_ks40[student_id] : visits_ks44[student_id]) << ")\n";
                        }
    
                        // Если занятие группы студента уже идет, то ожидаем окончания, и после окончания выходим
                        if (class_in_session && current_group == group) {
                            cv.wait(lock, [this]() { return !this->class_in_session || this->stop_flag; });
                            continue;
                        }
                        else {
                            // Если занятие еще не началось, ожидаем в течение S сек
                            bool started = cv.wait_until(lock, deadline, [this, group]() {
                                return this->class_in_session || this->stop_flag;
                            });
    
                            if (stop_flag) return;
    
                            if (!started) {
                                // Студент не дождался начала занятия и выходит
                                if (group == 1 && in_room_ks40[student_id]) {
                                    in_room_ks40[student_id] = false;
                                    present_ks40--;
                                    occupancy--;
                                }
                                else if (group == 2 && in_room_ks44[student_id]) {
                                    in_room_ks44[student_id] = false;
                                    present_ks44--;
                                    occupancy--;
                                }
                                std::cout << "Студент " << student_id << " из " << (group == 1 ? "КС-40" : "КС-44")
                                    << " не дождался начала (S=" << S << "s) — ушёл на 1s и вернётся\n";
                                
                                // уведомление для других студенотов, что места в классе еще есть
                                lock.unlock();
                                cv.notify_all();
                                std::this_thread::sleep_for(std::chrono::seconds(1));
                                break;
                            }
                            else {
                                // Если занятие группы студента идет, то ожидаем окончания, и после окончания выходим
                                if (class_in_session && current_group == group) {
                                    cv.wait(lock, [this]() { return !this->class_in_session || this->stop_flag; });
                                    if (stop_flag) return;
                                    continue;
                                }
                                else {
                                    // Если занятие НЕ группы студента идет, то выгоняем
                                    if (group == 1 && in_room_ks40[student_id]) {
                                        in_room_ks40[student_id] = false;
                                        present_ks40--;
                                        occupancy--;
                                    }
                                    else if (group == 2 && in_room_ks44[student_id]) {
                                        in_room_ks44[student_id] = false;
                                        present_ks44--;
                                        occupancy--;
                                    }
                                    std::cout << "Студент " << student_id << " из " << (group == 1 ? "КС-40" : "КС-44")
                                        << " был выгнан при старте занятия другой группы\n";
                                    
                                    // уведомляемЮ что состояние изменилось
                                    lock.unlock();
                                    cv.notify_all();
                                    std::this_thread::sleep_for(std::chrono::seconds(1));
                                    break;
                                }
                            }
                        }
                    }
                    else {
                        // Студент не может войти, когда нет мест или идет занятие чужой группы, ждем уведомления
                        cv.wait(lock);
                    }
                } 
            } 
    
            // Перед следующей попыткой проверяем флаг остановки
            if (stop_flag) return;
        }
    }
    
    /**
     * @brief Проверяет, все ли студенты выполнили требования по посещениям
     * 
     * @return true если все студенты посетили минимум 2 занятия, false в обратном случае
     */
    bool all_students_completed();
    
    /**
     * @brief Выводит подробную статистику посещений
     * 
     * Отображает количество посещений для каждого студента в удобочитаемом формате.
     */
    void print_statistics();
};
