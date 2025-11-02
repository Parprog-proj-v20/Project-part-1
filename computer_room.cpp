#include "computer_room.h"
#include <iostream>
#include <random>
#include <thread>
#include <chrono>
#include <iomanip>
#include <windows.h>

ComputerRoom::ComputerRoom()
    : visits_ks40(total_ks40, 0),
      visits_ks44(total_ks44, 0),
      in_room_ks40(total_ks40, false),
      in_room_ks44(total_ks44, false),
      attended_this_session_ks40(total_ks40, false),
      attended_this_session_ks44(total_ks44, false) {
}

/**
 * @brief Генерирует случайное время ожидания для принятия студентом решения поснщения занятия
 * 
 * @return Случайное число от 1 до 2 секунд
 */
int ComputerRoom::get_random_time() {
    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> dis(1, 2);
    return dis(gen);
}

/**
 * @brief Проверяет, можно ли начать занятие для указанной группы
 * 
 * @param group Номер группы (1 - КС-40, 2 - КС-44)
 * @return true если набралось достаточно студентов для начала занятия, false в обратном случае
 */
bool ComputerRoom::can_start_class(int group) {
    if (group == 1) return present_ks40 >= need_ks40;
    if (group == 2) return present_ks44 >= need_ks44;
    return false;
}

/**
 * @brief Запускает занятие для указанной группы, выгоняет студентов другой, отмечает посещения, поток преподавателя для завершения занятия через 5 секунд.
 * 
 * @param group Номер группы (1 - КС-40, 2 - КС-44)
 */
void ComputerRoom::start_class_locked(int group) {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    if (class_in_session || stop_flag) return;

    class_in_session = true;
    current_group = group;

    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Началось занятие для группы " << (group == 1 ? "КС-40" : "КС-44") << "\n";
    std::cout << "Статистика в начале занятия:\n";
    std::cout << "    В классе: " << occupancy << " студентов\n";
    std::cout << "    КС-40: " << present_ks40 << " студентов\n";
    std::cout << "    КС-44: " << present_ks44 << " студентов\n";
    std::cout << std::string(60, '=') << "\n";

    // Выгнать всех студентов другой группы
    if (group == 1) {
        for (int i = 0; i < total_ks44; ++i) {
            if (in_room_ks44[i]) {
                in_room_ks44[i] = false;
                present_ks44--;
                occupancy--;
                std::cout << "    Выгнан студент КС-44 №" << i << " (занятие для КС-40)\n";
            }
            attended_this_session_ks44[i] = false;
        }
        // Засчитать посещения студентам КС-40, находящимся в классе
        for (int i = 0; i < total_ks40; ++i) {
            if (in_room_ks40[i] && !attended_this_session_ks40[i]) {
                visits_ks40[i]++;
                attended_this_session_ks40[i] = true;
                std::cout << "    Посещение засчитано: КС-40 студент №" << i 
                          << " (всего: " << visits_ks40[i] << " посещений)\n";
            }
        }
    }
    else {
        for (int i = 0; i < total_ks40; ++i) {
            if (in_room_ks40[i]) {
                in_room_ks40[i] = false;
                present_ks40--;
                occupancy--;
                std::cout << "    Выгнан студент КС-40 №" << i << " (занятие для КС-44)\n";
            }
            attended_this_session_ks40[i] = false;
        }
        for (int i = 0; i < total_ks44; ++i) {
            if (in_room_ks44[i] && !attended_this_session_ks44[i]) {
                visits_ks44[i]++;
                attended_this_session_ks44[i] = true;
                std::cout << "    Посещение засчитано: КС-44 студент №" << i 
                          << " (всего: " << visits_ks44[i] << " посещений)\n";
            }
        }
    }

    // Оповестить всех о начале занятия
    cv.notify_all();

    // Запустить поток преподавателя для завершения занятия через 5 секунд
    std::thread([this]() {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        if (!stop_flag) {
            std::unique_lock<std::mutex> lock(this->mtx);
            if (!this->class_in_session) return;
            
            std::cout << "\n" << std::string(60, '=') << "\n";
            std::cout << "Завершение занятия для группы " << (this->current_group == 1 ? "КС-40" : "КС-44") << "\n";
            
            // Преподаватель выводит всех оставшихся студентов
            int exited_count = 0;
            for (int i = 0; i < this->total_ks40; ++i) {
                if (this->in_room_ks40[i]) {
                    this->in_room_ks40[i] = false;
                    this->present_ks40--;
                    this->occupancy--;
                    exited_count++;
                }
            }
            for (int i = 0; i < this->total_ks44; ++i) {
                if (this->in_room_ks44[i]) {
                    this->in_room_ks44[i] = false;
                    this->present_ks44--;
                    this->occupancy--;
                    exited_count++;
                }
            }
            
            std::cout << "    Вышло студентов после занятия: " << exited_count << "\n";
            std::cout << std::string(60, '=') << "\n";

            this->class_in_session = false;
            this->current_group = 0;

            // Сбросить флаги посещений для следующего занятия
            for (int i = 0; i < this->total_ks40; ++i) this->attended_this_session_ks40[i] = false;
            for (int i = 0; i < this->total_ks44; ++i) this->attended_this_session_ks44[i] = false;

            lock.unlock();
            this->cv.notify_all();
        }
    }).detach();
}


void ComputerRoom::stop() {
    stop_flag = true;
    cv.notify_all();
}

/**
 * @brief Моделирует поведение студента в компьютерном классе
 * 
 * @param group Идентификатор группы студента (1 - КС-40, 2 - КС-44)
 * @param student_id Уникальный идентификатор студента в пределах группы
 */
void ComputerRoom::student_behavior(int group, int student_id) {
    std::string group_name = (group == 1) ? "КС-40" : "КС-44";

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
                    occupancy++;
                    if (group == 1) { 
                        in_room_ks40[student_id] = true; 
                        present_ks40++; 
                    }
                    else { 
                        in_room_ks44[student_id] = true; 
                        present_ks44++; 
                    }

                    std::cout << group_name << ": студент №" << student_id << " вошёл\n";
                    std::cout << "[Всего в классе: " << occupancy << ", КС-40: " << present_ks40  << ", КС-44: " << present_ks44 << "]\n";

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
                        std::cout << group_name << " студент №" << student_id << " получил посещение (всего: " << (group == 1 ? visits_ks40[student_id] : visits_ks44[student_id]) << ")\n";
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
                            std::cout << group_name << " студент №" << student_id << " не дождался начала (ждал " << S << "с) → ушёл на 1с\n";
                            
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


bool ComputerRoom::all_students_completed() {
    std::lock_guard<std::mutex> lock(mtx);
    for (int v : visits_ks40) if (v < 2) return false;
    for (int v : visits_ks44) if (v < 2) return false;
    return true;
}

/**
 * @brief Выводит  статистику посещений
 */
void ComputerRoom::print_statistics() {
    std::lock_guard<std::mutex> lock(mtx);
    
    std::cout << "Итоговая статистика посещений\n\n";
    
    std::cout << "\nГруппа КС-40 (30 студентов):\n";
    for (int i = 0; i < total_ks40; ++i) {
        std::cout << "   Студент " << std::setw(2) << i << ": " << visits_ks40[i] << " посещений";
        if (visits_ks40[i] >= 2) {
            std::cout << "    (норма выполнена)";
        } else {
            std::cout << "     (необходимо " << (2 - visits_ks40[i]) << " ещё)";
        }
        std::cout << "\n";
    }
    
    std::cout << "\nГруппа КС-44 (24 студента):\n";
    std::cout << std::string(40, '-') << "\n";
    for (int i = 0; i < total_ks44; ++i) {
        std::cout << "   Студент " << std::setw(2) << i << ": " << visits_ks44[i] << " посещений";
        if (visits_ks44[i] >= 2) {
            std::cout << "    (норма выполнена)";
        } else {
            std::cout << "    (необходимо " << (2 - visits_ks44[i]) << " ещё)";
        }
        std::cout << "\n";
    }
    
    std::cout << std::string(70, '=') << "\n";
}

