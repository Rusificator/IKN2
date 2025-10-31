#include <iostream>  
#include <winsock2.h> 
#include <windows.h> 
#include <string> 
#include <vector>

#pragma comment (lib, "Ws2_32.lib")  
#define SRV_PORT 1234  
#define BUF_SIZE 64  
#define MAX_CLIENTS 100

using namespace std;

struct Employee {
    char name[20];
    int projects_completed;
    int overtime_hours;
    int efficiency;
    int initiatives;
};

struct ClientInfo {
    SOCKET socket;
    int id;
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

// Функция потока для обработки клиента
DWORD WINAPI ClientHandler(LPVOID lpParam) {
    ClientInfo* clientInfo = (ClientInfo*)lpParam;
    SOCKET clientSocket = clientInfo->socket;
    int clientId = clientInfo->id;

    char buf[BUF_SIZE] = { 0 };
    string msg = greeting;

    // Освобождаем память структуры, так как она больше не нужна
    delete clientInfo;

    cout << "Клиент #" << clientId << " подключен. Обработка в отдельном потоке." << endl;

    // Отправляем приветствие клиенту
    int msgSize = static_cast<int>(msg.size());
    send(clientSocket, (char*)&msg[0], msgSize, 0);

    // Основной цикл обработки запросов от клиента
    while (true) {
        // Получаем данные сотрудника от клиента
        int len = recv(clientSocket, buf, BUF_SIZE, 0);

        if (len == SOCKET_ERROR || len == 0) {
            // Клиент отключился или ошибка
            break;
        }

        Employee* emp = (Employee*)buf;
        string bonusCode = calculateBonus(*emp);

        cout << "Клиент #" << clientId << " | Сотрудник " << emp->name
            << ": " << bonusCode << endl;

        // Отправляем результат клиенту
        int bonusSize = static_cast<int>(bonusCode.size());
        send(clientSocket, (char*)&bonusCode[0], bonusSize, 0);
    }

    cout << "Клиент #" << clientId << " отключен." << endl;
    closesocket(clientSocket);
    return 0;
}

int main() {
    setlocale(LC_ALL, "rus");
    cout << "\t Упрощенный многопоточный сервер расчета премий\n";
    cout << "\t (Использует WinAPI потоки)\n";
    for (int i = 0; i < 50; i++)
        cout << "-";
    cout << endl;

    char buff[1024];
    if (WSAStartup(0x0202, (WSADATA*)&buff[0])) {
        cout << "Ошибка инициализации Winsock! \n" << WSAGetLastError();
        return -1;
    }

    SOCKET sListener;
    sockaddr_in sin;

    // Создание слушающего сокета
    sListener = socket(AF_INET, SOCK_STREAM, 0);
    if (sListener == INVALID_SOCKET) {
        cout << "Ошибка создания сокета! \n" << WSAGetLastError();
        WSACleanup();
        return -1;
    }

    // Настройка адреса сервера
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(SRV_PORT);

    // Привязка сокета к адресу
    if (bind(sListener, (sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR) {
        cout << "Ошибка привязки сокета! \n" << WSAGetLastError();
        closesocket(sListener);
        WSACleanup();
        return -1;
    }

    // Перевод сокета в режим прослушивания
    if (listen(sListener, SOMAXCONN) == SOCKET_ERROR) {
        cout << "Ошибка listen! \n" << WSAGetLastError();
        closesocket(sListener);
        WSACleanup();
        return -1;
    }

    cout << "Сервер запущен на порту " << SRV_PORT << endl;
    cout << "Ожидание подключений..." << endl;

    vector<HANDLE> clientThreads;
    int clientCounter = 0;

    // Основной цикл принятия подключений
    while (true) {
        sockaddr_in clntSin;
        int clntLen = sizeof(clntSin);

        // Принятие входящего соединения
        SOCKET clientSocket = accept(sListener, (sockaddr*)&clntSin, &clntLen);

        if (clientSocket == INVALID_SOCKET) {
            cout << "Ошибка accept! \n" << WSAGetLastError();
            continue;
        }

        clientCounter++;
        cout << "Принято новое подключение. ID клиента: " << clientCounter << endl;

        // Создаем структуру для передачи данных в поток
        ClientInfo* clientInfo = new ClientInfo;
        clientInfo->socket = clientSocket;
        clientInfo->id = clientCounter;

        // Создаем поток для обработки клиента
        HANDLE hThread = CreateThread(
            NULL,                   // Безопасность по умолчанию
            0,                      // Размер стека по умолчанию
            ClientHandler,          // Функция потока
            clientInfo,             // Параметры
            0,                      // Флаги создания
            NULL                    // ID потока не сохраняем
        );

        if (hThread == NULL) {
            cout << "Ошибка создания потока! \n" << GetLastError();
            delete clientInfo;
            closesocket(clientSocket);
        }
        else {
            clientThreads.push_back(hThread);

            // Очистка завершенных потоков
            for (auto it = clientThreads.begin(); it != clientThreads.end(); ) {
                DWORD exitCode;
                if (GetExitCodeThread(*it, &exitCode) && exitCode != STILL_ACTIVE) {
                    CloseHandle(*it);
                    it = clientThreads.erase(it);
                    cout << "Поток клиента завершен. Активных потоков: "
                        << clientThreads.size() << endl;
                }
                else {
                    ++it;
                }
            }
        }
    }

    // Закрытие handles потоков (на практике сюда не дойдет)
    for (HANDLE hThread : clientThreads) {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }

    closesocket(sListener);
    WSACleanup();
    return 0;
}