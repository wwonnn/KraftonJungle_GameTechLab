# walk_sound.mp3로 걷기 발자국 소리 넣기 예제

이 문서는 `JSEngine/Asset/Sound/walk_sound.mp3` 파일을 사용해서, 캐릭터가 걷기 애니메이션을 재생할 때 발자국 소리가 나게 만드는 가장 쉬운 예제를 설명합니다.

대상 독자:

- `AnimNotify`를 처음 써보는 사람
- 일단 "걸을 때 소리가 나는 것"부터 빠르게 확인하고 싶은 사람

이 예제는 가장 단순한 방식인 `UAnimNotify_PlaySFX`를 사용합니다.

## 1. 먼저 결론부터

현재 파일 위치는 아래입니다.

```txt
Asset/Sound/walk_sound.mp3
```

따라서 `Payload`에는 아래처럼 넣는 것이 가장 안전합니다.

```txt
SoundCue=Asset/Sound/walk_sound.mp3;SocketName=foot_l;VolumeMultiplier=1.0;Spatialized=true
```

중요:

- 이 엔진은 단순 이름만 넣으면 기본적으로 `Asset/Audio/` 쪽도 찾습니다.
- 하지만 지금 파일은 `Asset/Sound/`에 있으므로 `walk_sound`만 쓰지 말고 경로를 정확히 쓰는 것을 권장합니다.

## 2. 어떤 방식으로 만들 것인가

이번 예제는 걷기 애니메이션 안에 발이 바닥에 닿는 타이밍마다 `Notify`를 1개씩 넣는 방식입니다.

즉:

- 왼발 닿는 순간 `Notify` 1개
- 오른발 닿는 순간 `Notify` 1개

이 방법의 장점:

- 가장 단순합니다.
- 디버깅이 쉽습니다.
- `walk_sound.mp3`가 재생되는지 바로 확인할 수 있습니다.

## 3. 사용할 Notify 클래스

사용할 클래스는 아래입니다.

```txt
UAnimNotify_PlaySFX
```

이 클래스는 선택한 시점에 효과음을 1회 재생합니다.

## 4. 작업 전에 준비할 것

필요한 것:

- 걷기 애니메이션 시퀀스 1개
- 프리뷰 메시에 발 소켓이 있으면 더 좋음
- 예: `foot_l`, `foot_r`

소켓이 없어도 소리는 낼 수 있지만, `Spatialized=true`를 쓸 때는 소켓이 있는 편이 더 자연스럽습니다.

## 5. 전체 작업 순서

1. 걷기 애니메이션을 `Animation Sequence Viewer`에서 연다.
2. `Notifies` 섹션을 펼친다.
3. 필요하면 `Notify Track`를 하나 만든다.
4. 왼발 접지 타이밍에 `Notify`를 추가한다.
5. 오른발 접지 타이밍에 `Notify`를 추가한다.
6. 각 `Notify`의 `Notify Class`를 `UAnimNotify_PlaySFX`로 바꾼다.
7. `Payload`에 `walk_sound.mp3` 경로를 넣는다.
8. 재생해서 타이밍을 확인한다.
9. `Validation`에 오류가 없는지 확인하고 저장한다.

## 6. 걷기 애니메이션 열기

걷기용 `Animation Sequence`를 열어 주세요.

열린 뒤 아래 화면 요소를 확인합니다.

- 타임라인의 `Notifies`
- 아래쪽 `Selected Notify`
- 오른쪽 `Notify Debug`

## 7. Notify Track 준비

이미 트랙이 있으면 그대로 사용하면 됩니다.

없다면:

1. `Notifies` 헤더를 우클릭
2. `Add Track` 선택

처음에는 트랙 하나만 있어도 충분합니다.

예를 들어:

- `Track 1`

## 8. 발자국 Notify 2개 추가하기

## 8.1 왼발 타이밍 찾기

타임라인을 재생하면서 왼발이 바닥에 닿는 프레임을 찾습니다.

보통은 다음 기준으로 잡으면 됩니다.

- 발이 가장 아래에 내려온 순간
- 몸의 무게가 실리는 순간
- 발이 미끄러지기 직전 또는 직후가 아닌, 실제 접지 순간

## 8.2 왼발 Notify 추가

방법 1:

- 해당 시간 위치를 더블 클릭

방법 2:

- 해당 위치를 우클릭
- `Add Notify Here`

추가되면 새 마커를 클릭합니다.

## 8.3 오른발 Notify 추가

오른발도 같은 방식으로 추가합니다.

## 9. 왼발 Notify 설정

왼발 `Notify`를 클릭한 뒤, 아래 `Selected Notify` 패널에서 설정합니다.

### 9.1 Basic

- `Name`: `Footstep_L`
- `Type`: `Notify`
- `Notify Class`: `UAnimNotify_PlaySFX`

### 9.2 Timing

- `Time`: 왼발 접지 시점으로 조정
- `Duration`: `0`

### 9.3 Payload

아래 값을 넣습니다.

```txt
SoundCue=Asset/Sound/walk_sound.mp3;SocketName=foot_l;VolumeMultiplier=1.0;Spatialized=true
```

### 9.4 Visual

원하면 색을 바꿔서 발자국용 마커를 구분합니다.

예:

- 왼발: 초록

## 10. 오른발 Notify 설정

오른발 `Notify`도 같은 방식으로 설정합니다.

### 10.1 Basic

- `Name`: `Footstep_R`
- `Type`: `Notify`
- `Notify Class`: `UAnimNotify_PlaySFX`

### 10.2 Timing

- `Time`: 오른발 접지 시점
- `Duration`: `0`

### 10.3 Payload

오른발 소켓이 있다면 아래처럼 넣습니다.

```txt
SoundCue=Asset/Sound/walk_sound.mp3;SocketName=foot_r;VolumeMultiplier=1.0;Spatialized=true
```

오른발 소켓 이름이 없거나 확실하지 않다면, 임시로 아래처럼 써도 됩니다.

```txt
SoundCue=Asset/Sound/walk_sound.mp3;VolumeMultiplier=1.0;Spatialized=false
```

### 10.4 Visual

원하면 색을 다르게 줍니다.

예:

- 오른발: 파랑

## 11. 가장 추천하는 Payload 예제

### 11.1 소켓을 사용하는 경우

왼발:

```txt
SoundCue=Asset/Sound/walk_sound.mp3;SocketName=foot_l;VolumeMultiplier=1.0;Spatialized=true
```

오른발:

```txt
SoundCue=Asset/Sound/walk_sound.mp3;SocketName=foot_r;VolumeMultiplier=1.0;Spatialized=true
```

### 11.2 소켓 없이 먼저 소리만 확인하는 경우

```txt
SoundCue=Asset/Sound/walk_sound.mp3;VolumeMultiplier=1.0;Spatialized=false
```

처음에는 이 방식이 더 쉽습니다.

이유:

- 소켓 이름 오타 문제를 피할 수 있습니다.
- 파일 경로와 재생 자체가 되는지 먼저 확인할 수 있습니다.

## 12. 각 Payload 항목 의미

### 12.1 `SoundCue`

재생할 사운드 파일 경로 또는 키입니다.

이번 예제에서는 아래 값을 사용합니다.

```txt
Asset/Sound/walk_sound.mp3
```

### 12.2 `SocketName`

소리가 시작될 위치입니다.

예:

- `foot_l`
- `foot_r`

### 12.3 `VolumeMultiplier`

볼륨 크기 배수입니다.

예:

- `1.0`: 기본 크기
- `0.7`: 조금 작게
- `1.2`: 조금 크게

### 12.4 `Spatialized`

3D 위치 기반 재생 여부입니다.

- `true`: 발 위치에서 나는 느낌
- `false`: 단순 2D 효과음

처음 테스트는 `false`, 실제 연출은 `true`를 권장합니다.

## 13. 처음 테스트할 때 가장 쉬운 설정

소리가 안 나면 원인을 좁혀야 합니다.

가장 쉬운 1차 테스트 설정:

```txt
SoundCue=Asset/Sound/walk_sound.mp3;VolumeMultiplier=1.0;Spatialized=false
```

이 설정으로도 안 나오면 다음 중 하나일 가능성이 큽니다.

- `Notify` 타이밍이 재생 구간 밖에 있음
- `Notify Class`가 잘못됨
- 파일 경로 오타
- 오디오 시스템에서 파일을 읽지 못함

이 설정으로 소리가 나오면, 그 다음에 `SocketName`과 `Spatialized=true`를 추가하면 됩니다.

## 14. 소리가 안 날 때 체크할 것

### 14.1 `Notify Class`가 맞는가

반드시 아래여야 합니다.

```txt
UAnimNotify_PlaySFX
```

### 14.2 `Type`이 `Notify`인가

이번 예제는 `Notify State`가 아니라 일반 `Notify`입니다.

### 14.3 `SoundCue` 경로가 정확한가

이번 예제 기준 정답:

```txt
Asset/Sound/walk_sound.mp3
```

주의:

- `walk_sound`
- `walk_sound.mp3`
- `Sound/walk_sound.mp3`

처럼 줄여 쓰면 현재 구조에서는 원하는 파일을 못 찾을 수 있습니다.

### 14.4 `Validation`에 오류가 없는가

`Selected Notify > Validation` 또는 오른쪽 `Notify Debug > Validation`에서 확인합니다.

### 14.5 소켓 이름이 실제와 같은가

`Spatialized=true`이고 `SocketName`을 쓰는 경우:

- `foot_l`
- `foot_r`

이 이름이 프리뷰 메시에 실제로 있어야 합니다.

확실하지 않으면 일단 `Spatialized=false`로 확인하세요.

## 15. 추천하는 실제 작업 방법

아래 순서대로 하면 실패 확률이 가장 낮습니다.

### 단계 1. 소리만 먼저 확인

왼발/오른발 모두 아래처럼 넣습니다.

```txt
SoundCue=Asset/Sound/walk_sound.mp3;VolumeMultiplier=1.0;Spatialized=false
```

이 상태에서 재생했을 때 발 타이밍에 소리가 나는지 먼저 확인합니다.

### 단계 2. 타이밍 미세 조정

소리가 나면 각 `Notify` 마커를 조금씩 앞뒤로 옮겨서 가장 자연스러운 타이밍에 맞춥니다.

보통:

- 너무 빠르면 발이 닿기 전에 소리가 납니다.
- 너무 늦으면 바닥을 끄는 느낌이 납니다.

### 단계 3. 위치감 추가

그 다음 아래처럼 바꿉니다.

왼발:

```txt
SoundCue=Asset/Sound/walk_sound.mp3;SocketName=foot_l;VolumeMultiplier=1.0;Spatialized=true
```

오른발:

```txt
SoundCue=Asset/Sound/walk_sound.mp3;SocketName=foot_r;VolumeMultiplier=1.0;Spatialized=true
```

## 16. 예제용 최종 설정

## 16.1 왼발

- `Name`: `Footstep_L`
- `Type`: `Notify`
- `Notify Class`: `UAnimNotify_PlaySFX`
- `Payload`:

```txt
SoundCue=Asset/Sound/walk_sound.mp3;SocketName=foot_l;VolumeMultiplier=1.0;Spatialized=true
```

## 16.2 오른발

- `Name`: `Footstep_R`
- `Type`: `Notify`
- `Notify Class`: `UAnimNotify_PlaySFX`
- `Payload`:

```txt
SoundCue=Asset/Sound/walk_sound.mp3;SocketName=foot_r;VolumeMultiplier=1.0;Spatialized=true
```

## 17. 걷기 루프 애니메이션에서 Notify 개수 감각

한 걸음 사이클에 발 접지가 2번이면 보통 `Notify`도 2개입니다.

예:

- 왼발 1개
- 오른발 1개

만약 애니메이션이 긴 루프라면:

- 루프 한 사이클 안의 모든 접지 시점에 넣어야 합니다.

## 18. 나중에 확장하고 싶다면

이번 문서는 "한 파일을 바로 걷기 발소리로 재생하는 가장 쉬운 방법" 기준입니다.

추후 확장 가능:

- 표면별 발소리 분기: `UAnimNotify_FootstepSurfaceEvent`
- 걷기/달리기별 다른 사운드
- 왼발/오른발마다 볼륨 차이
- 랜덤 피치 또는 랜덤 샘플 재생 로직 추가

하지만 첫 단계에서는 `UAnimNotify_PlaySFX`로 먼저 성공시키는 것이 맞습니다.

## 19. 빠른 체크리스트

- 걷기 애니메이션을 열었는가?
- `Notify`를 발 접지 시점에 넣었는가?
- `Notify Class = UAnimNotify_PlaySFX` 인가?
- `Type = Notify` 인가?
- `SoundCue=Asset/Sound/walk_sound.mp3` 를 정확히 넣었는가?
- 소켓이 확실하지 않으면 `Spatialized=false`로 먼저 테스트했는가?
- `Validation`에 오류가 없는가?

## 20. 한 줄 요약

이번 예제에서 가장 중요한 설정은 이것입니다.

```txt
SoundCue=Asset/Sound/walk_sound.mp3
```

그리고 가장 쉬운 시작 설정은 이것입니다.

```txt
SoundCue=Asset/Sound/walk_sound.mp3;VolumeMultiplier=1.0;Spatialized=false
```

이 설정으로 먼저 소리가 나는지 확인한 뒤, `foot_l`, `foot_r` 소켓과 `Spatialized=true`를 붙이면 자연스러운 걷기 발자국 예제를 완성할 수 있습니다.
