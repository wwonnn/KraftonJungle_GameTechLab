#pragma once

class ScoreManager
{
public:
    static ScoreManager& Get()
    {
        static ScoreManager instance;
        return instance;
    }
    ScoreManager() = default;
    ~ScoreManager() = default;

    void AddScore(int points){
        score += points;
    } 
    int GetScore() const { return score; }

private:
    int score = 0;
};
