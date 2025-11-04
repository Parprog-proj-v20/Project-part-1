#pragma once
#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>

class ComputerRoom {
private:
  std::mutex mtx; 
    std::condition_variable cv; // Условная переменная для ожидания событий

    const int CAPACITY = 20; // Максимальная вместимость компьютерного класса
    const int TOTAL_KS40 = 30; // Общее кол-во студентов в КС-40
    const int TOTAL_KS44 = 24; // Общее кол-во студентов в КС-44
    const int NEED_KS40 = 15; // Необходимое кол-во студентов КС-40 для начала занятия
    const int NEED_KS44 = 12; // Необходимое кол-во студентов КС-44 для начала занятия

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
    int getRandomTime();
    bool canStartClass(int group);
    void startClassLocked(int group);


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
    void stop();

    
    /**
     * @brief Поведение студента в компьютерном классе
     * 
     * @param group Номер группы (1 - КС-40, 2 - КС-44)
     * @param student_id Идентификатор студента в группе
     * 
     * Метод моделирует поведение студента: попытки войти в класс, ожидание начала занятия, получение посещений и выход из класса.
     */
    void studentBehavior(int group, int student_id); 
    
    /**
     * @brief Проверяет, все ли студенты выполнили требования по посещениям
     * 
     * @return true если все студенты посетили минимум 2 занятия, false в обратном случае
     */
    bool allStudentsCompleted();
    
    /**
     * @brief Выводит подробную статистику посещений
     * 
     * Отображает количество посещений для каждого студента в удобочитаемом формате.
     */
    void printStatistics();
};
