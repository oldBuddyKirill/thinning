#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <set>

using namespace std;

static const int shift = 1e4;

static int getOutputPrecision();

static map<int, int> genValues(const size_t &samples, const float &mean, const float &stdDev);


int main()
{
    const float r = 25e3;   // расстояние до цели в метрах
    const float e = 50;     // допустимый разброс в метрах
    // считаем что r >> e
    // вычисляем заданную точность (в градусах) по т. косинусов
    const double p = std::round((std::acos((2.0 * r * r - e * e) / (2.0 * r * r)) * 180.0 / M_PI) * shift);
    auto precision = static_cast<int>(p);
    cout << "Заданная точность: " << static_cast<double>(precision) / shift << "°" << endl << endl;


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
    for (auto &&item: data) {
        if (item.second < 15)
            continue;
        cout << fixed << setw(11) << ++counter << ": "
             << setw(14) << setprecision(getOutputPrecision()) << static_cast<double>(item.first) / shift << " : "
             << item.second << endl;
    }
    cout << endl;


    // ############################## прореживание ##############################
    // объединение в группы с заданным шагом (точностью)
    map<int, map<int, int>> splitData;
    // поиск шага, который будет максимально близок к заданной точности
    // и на который 360*shift будет делиться без остатка
    // поиск проводим увеличивая точность (уменьшая шаг), за это отвечает флаг increasePrecision
    // алгоритм явно можно оптимизировать
    int step = precision;
    bool increasePrecision = true;
    while ((360 * shift % step)) {
        if (increasePrecision)
            --step;
        else
            ++step;
    }
    int currentStep = step;
//    int nextStep = currentStep + step;
    int checkCounter = 0;
    for (auto &&item: data) {
        while (item.first > currentStep) {
            currentStep += step;
        }
        if (currentStep > 360 * shift)
            break;
        splitData[currentStep][item.first] = item.second;
        ++checkCounter;
    }

    if (checkCounter != data.size()) {
        cerr << "Некоторые измерения были утеряны!" << endl;
    }

    cout << "SPLIT (" << splitData.size() << ") ==>\n";
    counter = 0;
    for (auto &&item: splitData) {
        cout << fixed << setw(11) << ++counter << ": "
             << setw(14) << setprecision(getOutputPrecision()) << static_cast<double>(item.first) / shift << " : "
             << item.second.size() << endl;
    }
    cout << endl;

    // запоминаем на каком промежутке было больше всего измерений; данную проверку выгоднее выполнять на этом этапе, чем на предыдущем
    size_t maxN = 0;
    for (auto &&item: splitData)
        if (item.second.size() > maxN)
            maxN = item.second.size();

    // в результате необходимо хранить только градусы, для отрисовки пеленгов
    set<float> result;
    // кф взят с потолка, надо попытаться подобрать экспериментальным путем
    // неплохо было бы добавить зависимость от заданной точности
    float n = std::max<float>(1.0, std::min(static_cast<float>(maxN), static_cast<float>(precision) / 30));
    for (auto &&item: splitData) {
        //
        //  также возможен вариант с логарифмами (будет добавлен чуть позже)
        //
        unsigned size = item.second.size();
        // идея в том, что чем меньше кф "k", тем меньше пеленгов на данном промежутке необходимо отобразить
        double k = (static_cast<float>(size) / static_cast<float>(maxN)) / static_cast<float>(n);
        auto advance = static_cast<unsigned>(std::round(1.0 / k));
        if (advance > size) {
            // если шаг больше размера мапы, то просто берем среднее значение из этого диапазона
            unsigned i = size / 2;
            int degree = std::next(item.second.begin(), i)->first;
            result.insert(static_cast<float>(degree) / shift);
        } else {
            for (unsigned i = advance / 2; i < size; i += advance) {
                int degree = std::next(item.second.begin(), i)->first;
                result.insert(static_cast<float>(degree) / shift);
            }
        }
    }


    // ############################## вывод результата ##############################
    cout << "RESULT (" << result.size() << ") ==>\n";
    counter = 0;
    float previous = 0;
    for (auto &&item: result) {
        cout << fixed << setw(11) << ++counter << ": "
             << setw(14) << setprecision(getOutputPrecision()) << item << ' ' << (item - previous) << endl;
        previous = item;
    }
    cout << endl;

    return 0;
}


static int getOutputPrecision()
{
    int i = 0;
    int s = shift;
    while (s /= 10)
        ++i;
    return i;
}


static map<int, int> genValues(const size_t &samples, const float &mean, const float &stdDev)
{
    map<int, int> data;
    if (mean < 0 || mean >= 360) {
        cerr << "Пик должен находиться в пределах от 0 до 360 градусов" << endl;
        return data;
    }

    random_device rd;
    mt19937 gen(rd());
    normal_distribution<float> distr(mean, stdDev);
    for (size_t i = 0; i < samples; ++i) {
        // считаем, что допустимая точность не может быть больше 0.0001 градуса
        int degree = static_cast<int>(std::round(distr(gen) * shift));
        if (degree < 0)
            degree += 360 * shift;
        else if (degree >= (360 * shift))
            degree -= 360 * shift;
        ++data[degree];
    }
    return data;
}
