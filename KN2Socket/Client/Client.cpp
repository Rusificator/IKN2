#include <iostream> 
#include <string>
#include <cstring>
#define _WINSOCK_DEPRECATED_NO_WARNINGS  
#include <WinSock2.h>
#include <Windows.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma warning(disable: 4996)

#define SRV_PORT 1234
#define CLNT_PORT 1235
#define BUF_SIZE 1024

using namespace std;

struct Employee {
    char name[20];
    int projects_completed;
    int overtime_hours;
    int efficiency;
    int initiatives;
};

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

    // Получаем адрес сервера от пользователя
    char serverAddr[256];
    cout << "Введите адрес сервера:" << endl;
    cout << " - 'localhost' для подключения к этому компьютеру" << endl;
    cout << " - IP-адрес (например, 192.168.1.100) для подключения по сети" << endl;
    cout << " - Имя компьютера в сети" << endl;
    cout << "Адрес сервера: ";
    cin.getline(serverAddr, sizeof(serverAddr));

    // Если пользователь ничего не ввел, используем localhost по умолчанию
    if (strlen(serverAddr) == 0) {
        strcpy(serverAddr, "localhost");
    }

    SOCKET s;
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        cout << "Ошибка создания сокета! \n" << WSAGetLastError();
        WSACleanup();
        return -1;
    }

    sockaddr_in clntSin, srvSin;

    clntSin.sin_family = AF_INET;
    clntSin.sin_addr.s_addr = 0;
    clntSin.sin_port = htons(CLNT_PORT);

    if (bind(s, (sockaddr*)&clntSin, sizeof(clntSin)) == SOCKET_ERROR) {
        cout << "Ошибка привязки сокета! \n" << WSAGetLastError();
        closesocket(s);
        WSACleanup();
        return -1;
    }

    // Настраиваем адрес сервера
    srvSin.sin_family = AF_INET;
    srvSin.sin_port = htons(SRV_PORT);

    // Преобразуем адрес сервера
    if (isalpha(serverAddr[0])) {
        // Если это доменное имя (localhost или имя компьютера)
        hostent* hp = gethostbyname(serverAddr);
        if (hp == NULL) {
            cout << "Не удается разрешить имя сервера: " << serverAddr << endl;
            closesocket(s);
            WSACleanup();
            return -1;
        }
        srvSin.sin_addr.s_addr = *((unsigned long*)hp->h_addr);
    }
    else {
        // Если это IP-адрес
        srvSin.sin_addr.s_addr = inet_addr(serverAddr);
    }

    cout << "Подключаемся к серверу " << serverAddr << "..." << endl;
    if (connect(s, (sockaddr*)&srvSin, sizeof(srvSin)) == SOCKET_ERROR) {
        cout << "Ошибка подключения к серверу! \n" << WSAGetLastError();
        cout << "Убедитесь, что:" << endl;
        cout << "1. Сервер запущен на указанном адресе" << endl;
        cout << "2. Адрес сервера указан правильно" << endl;
        cout << "3. Брандмауэр разрешает подключения на порту " << SRV_PORT << endl;
        closesocket(s);
        WSACleanup();
        return -1;
    }
    cout << "Подключение установлено!" << endl;

    int len = 0;
    char buf[BUF_SIZE] = { 0 };

    // Получаем приветствие от сервера
    len = recv(s, buf, BUF_SIZE, 0);
    if (len == SOCKET_ERROR) {
        cout << "Ошибка приема сообщения! \n" << WSAGetLastError();
        closesocket(s);
        WSACleanup();
        return -1;
    }

    if (len >= 0 && len < BUF_SIZE) {
        buf[len] = 0;
    }
    else if (len >= BUF_SIZE) {
        buf[BUF_SIZE - 1] = 0;
    }
    cout << "Сообщение от сервера: " << buf << endl;

    while (true) {
        // Ввод данных сотрудника
        Employee E;
        cout << "\nВведите данные сотрудника:" << endl;
        cout << "Введите имя сотрудника: ";
        cin.getline(E.name, 20);

        // Проверяем, не хочет ли пользователь выйти
        if (strcmp(E.name, "exit") == 0 || strcmp(E.name, "quit") == 0) {
            cout << "Завершение работы..." << endl;
            break;
        }

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
        int sent = send(s, (char*)&E, sizeof(Employee), 0);
        if (sent == SOCKET_ERROR) {
            cout << "Ошибка отправки данных! \n" << WSAGetLastError();
            break;
        }

        // Получаем код премии от сервера
        len = recv(s, buf, BUF_SIZE, 0);
        if (len == SOCKET_ERROR || len == 0) {
            cout << "Сервер разорвал соединение" << endl;
            break;
        }

        // Безопасная обработка полученных данных
        if (len >= 0 && len < BUF_SIZE) {
            buf[len] = 0;
        }
        else if (len >= BUF_SIZE) {
            buf[BUF_SIZE - 1] = 0;
        }

        string bonusCode = string(buf);
        string bonusMessage = getBonusMessage(bonusCode);

        // Выводим результат
        cout << "\n" << string(40, '=') << endl;
        cout << "Результат для сотрудника " << E.name << ": " << bonusMessage << endl;
        cout << string(40, '=') << "\n" << endl;
    }

    closesocket(s);
    WSACleanup();
    return 0;
}