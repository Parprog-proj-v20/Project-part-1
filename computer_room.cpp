#include "computer_room.h"
#include <iostream>
#include <random>
#include <thread>
#include <chrono>
#include <iomanip>

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


bool ComputerRoom::all_students_completed() {
    std::lock_guard<std::mutex> lock(mtx);
    for (int v : visits_ks40) if (v < 2) return false;
    for (int v : visits_ks44) if (v < 2) return false;
    return true;
}

