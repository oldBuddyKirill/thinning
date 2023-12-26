#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <vector>

using namespace std;

static const int shift = 1e4;

static const int getOutputPrecision() {
    int  i = 0;
    int s = shift;
    while (s /= 10)
        ++i;
    return i;
}

map<int, int> genValues(size_t samples, float mean, float stdDev);

int main()
{
    const float r = 25e3;   // расстояние до цели в метрах
    const float e = 50;     // допустимый разброс в метрах
    // считаем что r >> e
    // вычисляем заданную точнось (в градусах) по т. косинусов
    const int precision = std::round((std::acos((2*r*r - e*e) / (2*r*r)) * 180 / M_PI) * shift);
    cout << "Заданная точность: " << static_cast<double>(precision)/shift << "°" << endl << endl;


    // ############################## заполнение тестовыми данными ##############################
    size_t samples = 1e4;
    const float mean = 5;
    const float stdDev = 0.5;    // standard deviation
    // ключ - градус [0, 360), умноженный на shift
    // значение - количество измерений
    map<int, int> data = genValues(samples, mean, stdDev);    // можно также добавить несколько измерений

    cout << "INITIAL (" << data.size() << ") ==>\n"
         << "Distribution for " << samples << " samples:" << endl;
    int counter = 0;
    for (auto &&item : data) {
        if (item.second < 18)
            continue;
        cout << fixed << setw(11) << ++counter << ": "
             << setw(14) << setprecision(getOutputPrecision()) << static_cast<double>(item.first)/shift << " : " << item.second << endl;
    }
    cout << endl;


    // ############################## прореживание ##############################
    // объединение в группы с заданным шагом (точностью)
    map<int, map<int, int>> splittedData;
    // поиск шага, который будет максимально близок к заданной точности и, на который 360*shift будет делиться без остатка
    // поиск проводим увеличивая точность (уменьшая шаг), за это отвечает флаг increasePrecision
    // алгоритм явно можно оптимизировать
    int step = precision;
    bool increasePrecision = true;
    while ((360*shift % step)) {
        if (increasePrecision)
            --step;
        else
            ++step;
    }
    int currentStep = 0;
    int nextStep = currentStep + step;
    int checkCounter = 0;
    for (auto &&item : data) {
        while (item.first > nextStep) {
            currentStep = nextStep;
            nextStep += step;
        }
        if (currentStep > 360 * shift)
            break;
        splittedData[currentStep][item.first] = item.second;
        ++checkCounter;
    }

    if (checkCounter != data.size()) {
        cerr << "Некоторые измерения были утеряны!" << endl;
    }

    cout << "SPLITTED (" << splittedData.size() << ") ==>\n";
    counter = 0;
    for (auto &&item : splittedData) {
        cout << fixed << setw(11) << ++counter << ": "
             << setw(14) << setprecision(getOutputPrecision()) << static_cast<double>(item.first)/shift << " : " << item.second.size() << endl;
    }
    cout << endl;

    // запоминаем на каком промежутке было больше всего измерений; данную проверку выгоднее выполнять на этом этапе, чем на предыдущем
    int maxN = 0;
    for (auto &&item : splittedData)
        if (item.second.size() > maxN)
            maxN = item.second.size();

    // в результате необходимо хранить только градусы, для отрисовки пеленгов
    set<float> result;
    // кф взят с потолка, надо попытаться подобрать экспериментальным путем
    // неплохо было бы добавить зависимость от заданной точности
//    int n = std::max(1, std::min<int>(maxN, std::round(precision/std::sqrt(shift))));
    int n = std::max(1, std::min<int>(maxN, 100));
    for (auto &&item : splittedData) {
        // идея в том, что чем меньше кф "k", тем меньше пеленгов на данном промежутке необходимо отобразить
        size_t size = item.second.size();
        float k = (size / static_cast<float>(maxN)) / n;
        int advance = std::round(1.0/k);
        if (advance > size) {
            // елси шаг больше размера мапы, то просто берем среднее значение из этого диапазона
            int i = size / 2;
            int degree = std::next(item.second.begin(), i)->first;
            result.insert(static_cast<float>(degree)/shift);
        } else {
            for (int i = advance/2; i < size; i += advance) {
                int degree = std::next(item.second.begin(), i)->first;
                result.insert(static_cast<float>(degree)/shift);
            }
        }
//        for (size_t i = 0; i < size; i += advance) {
//            int degree = std::next(item.second.begin(), i)->first;
//            result.insert(static_cast<float>(degree)/shift);
//        }
    }


    // ############################## вывод результата ##############################
    cout << "RESULT (" << result.size() << ") ==>\n";
    counter = 0;
    float previous = 0;
    for (auto &&item : result) {
        cout << fixed << setw(11) << ++counter << ": "
             << setw(14) << setprecision(getOutputPrecision()) << item << ' ' << (item - previous) << endl;
        previous = item;
    }
    cout << endl;

    return 0;
}


map<int, int> genValues(size_t samples, const float mean, const float stdDev)
{
    map<int, int> data;
    if (mean < 0 || mean >= 360) {
        cerr << "Пик должен находиться в пределах от 0 до 360 градусов" << endl;
        return data;
    }

    random_device rd;
    mt19937 gen(rd());
    normal_distribution<float> distr(mean, stdDev);
    for (size_t i; i < samples; ++i) {
        // считаем, что допустимая точность не может быть больше 0.0001 градуса
        int degree = std::round(distr(gen)*shift);
        if (degree < 0 )
            degree += 360*shift;
        else if (degree >= (360*shift))
            degree -= 360*shift;
        ++data[degree];
    }
    return data;
}
