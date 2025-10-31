#include <iostream>  
#include <winsock2.h> 
#include <windows.h> 
#include <string> 
#include <thread>
#include <vector>

#pragma comment (lib, "Ws2_32.lib")  
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

const string greeting = "Сервер готов к расчету премий";

string calculateBonus(const Employee& emp) {
    int score = emp.projects_completed + emp.overtime_hours +
        emp.efficiency + emp.initiatives;

    if (score < 10) return "NO_BONUS";
    else if (score >= 10 && score < 20) return "STANDARD_BONUS";
    else if (score >= 20 && score < 30) return "MEDIUM_BONUS";
    else return "HIGH_BONUS";
}

void handleClient(SOCKET sNew) {
    cout << "Новый подключенный клиент! " << endl;

    int len;
    char buf[BUF_SIZE] = { 0 };
    string msg = greeting;

    // Отправляем приветствие один раз
    int msgSize = static_cast<int>(msg.size());
    send(sNew, (char*)&msg[0], msgSize, 0);

    while (true) {
        // Получаем данные сотрудника от клиента
        len = recv(sNew, (char*)buf, BUF_SIZE, 0);
        if (len == SOCKET_ERROR || len == 0) {
            break;
        }

        Employee* emp = (Employee*)(&buf[0]);
        string bonusCode = calculateBonus(*emp);

        cout << "Сотрудник " << emp->name << ": " << bonusCode << endl;

        // Отправляем только код премии
        int bonusSize = static_cast<int>(bonusCode.size());
        send(sNew, (char*)&bonusCode[0], bonusSize, 0);
    }

    cout << "Клиент отключен \n";
    closesocket(sNew);
}

int main() {
    setlocale(LC_ALL, "rus");
    cout << "\t Сервер расчета премий\n";
    for (int i = 0; i < 30; i++)
        cout << "-";
    cout << endl;

    char buff[1024];
    if (WSAStartup(0x0202, (WSADATA*)&buff[0])) {
        cout << "Ошибка инициализации! \n" << WSAGetLastError();
        return -1;
    }

    SOCKET sListener;
    sockaddr_in sin, clntSin;

    sListener = socket(AF_INET, SOCK_STREAM, 0);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port = htons(SRV_PORT);
    bind(sListener, (sockaddr*)&sin, sizeof(sin));

    listen(sListener, SOMAXCONN);

    while (true) {
        int clntLen = sizeof(clntSin);
        SOCKET sNew = accept(sListener, (sockaddr*)&clntSin, &clntLen);

        if (sNew == INVALID_SOCKET) {
            cout << "Ошибка принятия соединения! \n" << WSAGetLastError();
            continue;
        }

        // Запускаем новый поток для обработки клиента
        thread clientThread(handleClient, sNew);
        clientThread.detach(); // Отсоединяем поток для самостоятельной работы
    }

    closesocket(sListener);
    WSACleanup();
    return 0;
}