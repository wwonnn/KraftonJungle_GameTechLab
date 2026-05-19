#pragma once

class UField
{
public:
    UField() = default;
    explicit UField(const char* InName)
        : Name(InName)
    {
    }

    virtual ~UField() = default;

    const char* GetName() const { return Name; }

protected:
    void SetName(const char* InName) { Name = InName; }

private:
    const char* Name = nullptr;
};
