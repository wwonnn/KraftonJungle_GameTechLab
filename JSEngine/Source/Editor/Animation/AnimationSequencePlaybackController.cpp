#include "Editor/Animation/AnimationSequencePlaybackController.h"

#include "Editor/Animation/AnimationSequencePreviewScene.h"
#include "Editor/Animation/AnimationSequenceViewerUtils.h"

#include <algorithm>
#include <cmath>

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

void FAnimationSequencePlaybackController::Initialize(UAnimSequence* InSequence)
{
    Sequence = InSequence;
    CurrentTime = 0.0f;
    PlayRate = 1.0f;
    bPlaying = false;
    bLooping = true;
    bPoseDirty = false;
}

void FAnimationSequencePlaybackController::Shutdown()
{
    Sequence = nullptr;
    CurrentTime = 0.0f;
    PlayRate = 1.0f;
    bPlaying = false;
    bLooping = true;
    bPoseDirty = false;
}

bool FAnimationSequencePlaybackController::HasSequence() const
{
    return AnimationSequenceViewer::IsLiveObject(Sequence);
}

void FAnimationSequencePlaybackController::Tick(float DeltaTime, FAnimationSequencePreviewScene* PreviewScene)
{
    if (PreviewScene && PreviewScene->HasValidPreview())
    {
        // PlaybackController owns time progression; PreviewScene owns how that
        // time is pushed into the preview component and viewport.
        PreviewScene->ApplyPlaybackSettings(bLooping, PlayRate, bPlaying);

        if (!bPlaying && !bPoseDirty)
        {
            return;
        }

        if (bPoseDirty)
        {
            PreviewScene->SetCurrentTime(CurrentTime);
            CurrentTime = PreviewScene->GetCurrentTime();
            bPoseDirty = false;
            if (!bPlaying)
            {
                return;
            }
        }

        PreviewScene->RefreshPreviewPose(bPlaying ? DeltaTime : 0.0f);
        CurrentTime = PreviewScene->GetCurrentTime();
        return;
    }

    if (!bPlaying && !bPoseDirty)
    {
        return;
    }

    if (bPoseDirty)
    {
        CurrentTime = ClampCurrentTime(CurrentTime);
        bPoseDirty = false;
        if (!bPlaying)
        {
            return;
        }
    }

    AdvanceWithoutPreview(DeltaTime);
}

void FAnimationSequencePlaybackController::SetCurrentTime(float InTime)
{
    CurrentTime = ClampCurrentTime(InTime);
    bPoseDirty = true;
}

float FAnimationSequencePlaybackController::GetLength() const
{
    return AnimationSequenceViewer::GetSequenceLengthSeconds(Sequence);
}

FFrameRate FAnimationSequencePlaybackController::GetFrameRate() const
{
    return AnimationSequenceViewer::GetSequenceFrameRate(Sequence);
}

int32 FAnimationSequencePlaybackController::GetFrameCount() const
{
    return AnimationSequenceViewer::GetSequenceFrameCount(Sequence);
}

int32 FAnimationSequencePlaybackController::GetCurrentFrameIndex() const
{
    return AnimationSequenceViewer::TimeToFrameIndex(CurrentTime, GetFrameRate(), GetFrameCount());
}

void FAnimationSequencePlaybackController::Play()
{
    if (!HasSequence())
    {
        bPlaying = false;
        return;
    }

    bPlaying = true;
}

void FAnimationSequencePlaybackController::Pause()
{
    bPlaying = false;
}

void FAnimationSequencePlaybackController::Stop()
{
    Pause();
    JumpToStart();
}

void FAnimationSequencePlaybackController::StepToNextFrame()
{
    const int32 FrameCount = GetFrameCount();
    if (FrameCount <= 0)
    {
        return;
    }

    Pause();
    SetCurrentTime(
        AnimationSequenceViewer::FrameIndexToTime(
            std::min(GetCurrentFrameIndex() + 1, FrameCount - 1),
            GetFrameRate()));
}

void FAnimationSequencePlaybackController::StepToPreviousFrame()
{
    const int32 FrameCount = GetFrameCount();
    if (FrameCount <= 0)
    {
        return;
    }

    Pause();
    SetCurrentTime(
        AnimationSequenceViewer::FrameIndexToTime(
            std::max(GetCurrentFrameIndex() - 1, 0),
            GetFrameRate()));
}

void FAnimationSequencePlaybackController::JumpToStart()
{
    Pause();
    SetCurrentTime(0.0f);
}

void FAnimationSequencePlaybackController::JumpToEnd()
{
    const int32 FrameCount = GetFrameCount();
    Pause();

    if (FrameCount > 1)
    {
        SetCurrentTime(AnimationSequenceViewer::FrameIndexToTime(FrameCount - 1, GetFrameRate()));
        return;
    }

    SetCurrentTime(GetLength());
}

void FAnimationSequencePlaybackController::ApplyPendingPose(FAnimationSequencePreviewScene* PreviewScene)
{
    if (!PreviewScene || !PreviewScene->HasValidPreview())
    {
        return;
    }

    PreviewScene->ApplyPlaybackSettings(bLooping, PlayRate, bPlaying);
    PreviewScene->SetCurrentTime(CurrentTime);
    CurrentTime = PreviewScene->GetCurrentTime();
    bPoseDirty = false;
}

float FAnimationSequencePlaybackController::ClampCurrentTime(float InTime) const
{
    return std::clamp(InTime, 0.0f, GetLength());
}

void FAnimationSequencePlaybackController::AdvanceWithoutPreview(float DeltaTime)
{
    const float Length = GetLength();
    CurrentTime += DeltaTime * PlayRate;

    if (bLooping && Length > 0.0f)
    {
        CurrentTime = std::fmod(CurrentTime, Length);
        if (CurrentTime < 0.0f)
        {
            CurrentTime += Length;
        }
        return;
    }

    CurrentTime = std::clamp(CurrentTime, 0.0f, Length);
    if ((PlayRate >= 0.0f && CurrentTime >= Length) || (PlayRate < 0.0f && CurrentTime <= 0.0f))
    {
        bPlaying = false;
    }
}
