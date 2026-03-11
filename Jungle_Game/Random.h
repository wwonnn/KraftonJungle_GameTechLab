#include <random>

class Random
{
public:
    static void Initialize()
    {
        std::random_device rd;
        GetEngine().seed(rd());
    }

    static int Range(int min, int max)
    {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(GetEngine());
    }

    static float Range(float min, float max)
    {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(GetEngine());
    }

private:
    static std::mt19937& GetEngine()
    {
        static std::mt19937 engine;
        return engine;
    }
};
