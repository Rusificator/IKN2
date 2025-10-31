#include <iostream> 
#include <string>
#define _WINSOCK_DEPRECATED_NO_WARNINGS  
#include <WinSock2.h>
#include <Windows.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma warning(disable: 4996)

#define SRV_HOST "localhost"
#define SRV_PORT 1234

#define BUF_SIZE 64

using namespace std;

struct Employee {
    char name[20];
    int projects_completed;
    int overtime_hours;
    int efficiency;
    int initiatives;
};

// Функция для преобразования кода премии в читаемое сообщение
string getBonusMessage(const string& bonusCode) {
    if (bonusCode == "NO_BONUS") return "Премия не назначена";
    else if (bonusCode == "STANDARD_BONUS") return "Стандартная премия";
    else if (bonusCode == "MEDIUM_BONUS") return "Средняя премия";
    else if (bonusCode == "HIGH_BONUS") return "Высокая премия";
    else return "Неизвестный тип премии: " + bonusCode;
}

int main() {
    setlocale(LC_ALL, "rus");
    cout << "\t Клиент для расчета премий\n";
    for (int i = 0; i < 30; i++)
        cout << "-";
    cout << endl;

    char buff[1024];
    if (WSAStartup(0x0202, (WSADATA*)&buff[0])) {
        cout << "Ошибка инициализации! \n" << WSAGetLastError();
        return -1;
    }

    SOCKET s;
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        cout << "Ошибка создания сокета! \n" << WSAGetLastError();
        return -1;
    }

    sockaddr_in  srvSin;

   

    hostent* hp;
    srvSin.sin_family = AF_INET;
    srvSin.sin_port = htons(SRV_PORT);
    hp = gethostbyname(SRV_HOST);
    ((unsigned long*)&srvSin.sin_addr)[0] = ((unsigned long**)hp->h_addr_list)[0][0];

    cout << "Подключаемся к серверу..." << endl;
    if (connect(s, (sockaddr*)&srvSin, sizeof(srvSin)) == SOCKET_ERROR) {
        cout << "Ошибка подключения к серверу! \n" << WSAGetLastError();
        closesocket(s);
        WSACleanup();
        return -1;
    }
    cout << "Подключение установлено!" << endl;

    int len = 0;
    char buf[BUF_SIZE] = { 0 };

    // Получаем приветствие от сервера
    len = recv(s, (char*)&buf, BUF_SIZE, 0);
    if (len == SOCKET_ERROR) {
        cout << "Ошибка приема сообщения! \n" << WSAGetLastError();
        closesocket(s);
        WSACleanup();
        return -1;
    }

    // Безопасная установка нулевого терминатора
    if (len >= 0 && len < BUF_SIZE) {
        buf[len] = 0;
    }
    else if (len >= BUF_SIZE) {
        buf[BUF_SIZE - 1] = 0; // Гарантируем, что не выйдем за границы
    }
    cout << "Сообщение от сервера: " << buf << endl;

    while (true) {
        // Ввод данных сотрудника
        Employee E;
        cout << "\nВведите данные сотрудника:" << endl;
        cout << "Введите имя сотрудника: ";
        cin.getline(E.name, 20);
        cout << "Введите количество завершенных проектов: ";
        cin >> E.projects_completed;
        cout << "Введите количество сверхурочных часов: ";
        cin >> E.overtime_hours;
        cout << "Введите показатель эффективности (0-10): ";
        cin >> E.efficiency;
        cout << "Введите количество инициатив: ";
        cin >> E.initiatives;

        string ss;
        getline(cin, ss); // Очистка буфера

        // Отправляем данные сотрудника на сервер
        send(s, (char*)&E, sizeof(Employee), 0);

        // Получаем код премии от сервера
        len = recv(s, (char*)&buf, BUF_SIZE, 0);
        if (len == SOCKET_ERROR) {
            cout << "Ошибка приема сообщения! \n" << WSAGetLastError();
            break;
        }

        // Безопасная установка нулевого терминатора
        if (len >= 0 && len < BUF_SIZE) {
            buf[len] = 0;
        }
        else if (len >= BUF_SIZE) {
            buf[BUF_SIZE - 1] = 0;
        }
        string bonusCode = string(buf);
        string bonusMessage = getBonusMessage(bonusCode);
        

        // Выводим красивое сообщение на основе кода от сервера
        cout << "\n" << string(40, '=') << endl;
        cout << "Результат для сотрудника " << E.name << ": " << bonusMessage << endl;
        cout << string(40, '=') << "\n" << endl;
    }

    closesocket(s);
    WSACleanup();
    return 0;
}
