# Animation Sequence Viewer AnimNotify 사용 가이드

이 문서는 `Animation Sequence Viewer`에서 `AnimNotify`를 처음 사용하는 사람을 대상으로 작성했습니다.
목표는 다음 3가지를 혼자 할 수 있게 만드는 것입니다.

1. 애니메이션 타임라인에 `Notify`를 추가한다.
2. `Notify`와 `Notify State`의 차이를 이해하고 맞게 사용한다.
3. 내장 `AnimNotify` 클래스와 `Payload`를 설정하고 검증한다.

## 1. AnimNotify가 무엇인가

`AnimNotify`는 애니메이션이 재생되는 특정 시점에 이벤트를 발생시키는 기능입니다.

예를 들면 다음과 같은 작업에 사용합니다.

- 발소리 재생
- 공격 판정 시작/종료
- VFX 재생
- 카메라 흔들림 발생
- 게임플레이 이벤트 전송

즉, "애니메이션의 몇 초 지점에서 무엇을 할지"를 데이터로 넣는 기능이라고 보면 됩니다.

## 2. 먼저 알아야 할 핵심 개념

### 2.1 Notify

한 시점에서 한 번만 실행됩니다.

예시:

- 검이 맞는 순간 효과음 1회 재생
- 0.42초에 카메라 흔들림 1회 실행
- 특정 타이밍에 게임플레이 이벤트 1회 전송

### 2.2 Notify State

시작 시점과 종료 시점을 가지는 구간형 이벤트입니다.

예시:

- 공격 판정이 켜져 있는 시간 구간
- 루프 사운드가 재생되는 시간 구간
- 무적 판정이 유지되는 시간 구간

### 2.3 Payload

`Notify`가 실행될 때 함께 전달되는 추가 데이터 문자열입니다.

이 엔진에서는 보통 아래 형식으로 저장됩니다.

```txt
Key=Value;Key=Value;Key=Value
```

예시:

```txt
SoundCue=Footstep_Stone;SocketName=foot_l;VolumeMultiplier=0.9;Spatialized=true
```

## 3. 화면에서 어디를 보면 되는가

`Animation Sequence Viewer`에서 `AnimNotify` 작업은 주로 아래 3곳에서 합니다.

### 3.1 Sequencer 타임라인

여기서 `Notify` 마커를 추가, 선택, 이동, 삭제합니다.

- `Notifies` 섹션 아래에 트랙이 표시됩니다.
- 각 트랙 행 위에 `Notify` 마커가 보입니다.
- `Notify State`는 길이가 있는 막대처럼 보입니다.

### 3.2 왼쪽 아래 `Selected Notify`

선택한 `Notify`의 상세 설정을 편집합니다.

여기서 수정할 수 있는 대표 항목:

- `Name`
- `Type`
- `Notify Class`
- `Time`
- `Duration`
- `Payload`
- `Color`

### 3.3 오른쪽 아래 `Notify Debug`

검증과 디버그 확인용 영역입니다.

탭 구성:

- `Validation`
- `Recent Fired`
- `Event Log`

처음 사용하는 사람은 특히 `Validation` 탭을 자주 보는 것이 좋습니다.

## 4. 가장 기본적인 사용 순서

처음에는 아래 순서대로 작업하면 됩니다.

1. 애니메이션 시퀀스를 연다.
2. `Notifies` 섹션이 접혀 있으면 펼친다.
3. 필요하면 `Notify Track`를 추가한다.
4. 원하는 시간 위치에 `Notify`를 만든다.
5. 생성된 마커를 클릭한다.
6. `Selected Notify` 패널에서 `Type`, `Notify Class`, `Payload`를 설정한다.
7. `Validation`에서 오류와 경고를 확인한다.
8. 저장한다.

## 5. Notify Track 만들기와 관리

`Notify`는 트랙 안에 들어갑니다.

### 5.1 Track 추가

`Notifies` 헤더를 우클릭하면 `Add Track` 메뉴가 있습니다.

추가하면 기본 이름은 아래처럼 생성됩니다.

- `Track 1`
- `Track 2`
- `Track 3`

### 5.2 Track 이름 변경

트랙 행을 우클릭한 뒤 `Rename Track`을 선택합니다.

### 5.3 Track 이동

트랙 행 우클릭 메뉴:

- `Move Up`
- `Move Down`

### 5.4 Track 삭제

비어 있는 트랙만 삭제할 수 있습니다.

트랙에 `Notify`가 하나라도 있으면 삭제되지 않습니다.

## 6. Notify 추가하는 방법

이 에디터에는 여러 추가 방법이 있습니다.

### 6.1 가장 쉬운 방법: 더블 클릭

타임라인의 `Notify Track` 행에서 원하는 시간 위치를 더블 클릭하면 그 위치에 `Notify`가 추가됩니다.

생성 직후 기본값은 대체로 다음과 같습니다.

- `Name`: `Notify`
- `Type`: `Notify`
- `Duration`: `0`
- `Notify Class`: 기본 `UAnimNotify`

### 6.2 우클릭으로 추가

트랙 행의 빈 공간을 우클릭하면 다음 메뉴가 나옵니다.

- `Add Notify Here`
- `Paste Notify Here`

### 6.3 상세 패널 버튼으로 추가

`Selected Notify` 영역 상단 툴바의 `Add Notify` 버튼을 눌러도 추가할 수 있습니다.

이 경우 현재 시간(`CurrentTime`) 기준으로 선택된 트랙, 또는 기본 트랙에 추가됩니다.

## 7. Notify 선택 후 할 수 있는 편집

## 7.1 기본 정보 수정

`Selected Notify > Basic`

- `Name`: 사람이 읽기 쉬운 이름을 넣습니다.
- `Type`: `Notify` 또는 `Notify State`
- `Notify Class`: 실제 실행 클래스를 선택합니다.

권장 사항:

- 이름은 기능이 보이게 짓는 것이 좋습니다.
- 예: `Footstep_L`, `AttackStart`, `SwordTrail`, `InvulnWindow`

### 7.2 시간 수정

`Selected Notify > Timing`

- `Time`: 시작 시점
- `Duration`: 길이

주의:

- `Notify`는 보통 `Duration = 0`으로 둡니다.
- `Notify State`는 `Duration > 0`이어야 의미가 있습니다.

### 7.3 위치 드래그

마커를 드래그하면 시간을 옮길 수 있습니다.

### 7.4 Notify State 길이 조절

`Notify State`는 마커 오른쪽 끝을 드래그해서 종료 시점을 늘리거나 줄일 수 있습니다.

### 7.5 색상 변경

`Selected Notify > Visual > Color`에서 마커 색을 바꿀 수 있습니다.

색상은 기능 분류에 맞춰 통일하면 보기 편합니다.

예:

- 오디오: 파랑
- 공격 관련: 빨강
- 이동/발소리: 초록
- 카메라/연출: 노랑

## 8. 우클릭 메뉴로 할 수 있는 일

### 8.1 Notify 마커 우클릭

- `Duplicate`
- `Copy`
- `Delete`
- `Move To Track`

### 8.2 Track 행 우클릭

- `Rename Track`
- `Paste Notify Here`
- `Move Selected Notify Here`
- `Move Up`
- `Move Down`
- `Delete Empty Track`

## 9. Copy / Paste / Duplicate 차이

### 9.1 Duplicate

선택한 `Notify`를 같은 트랙에 복제합니다.

- 새 `StableId`가 생성됩니다.
- 시간은 현재 프레임 레이트 기준으로 약간 뒤로 밀려서 추가됩니다.

### 9.2 Copy

선택한 `Notify`를 클립보드에 복사합니다.

### 9.3 Paste

복사해 둔 `Notify`를 현재 시간 또는 우클릭한 위치에 붙여 넣습니다.

복사되는 항목:

- `Type`
- `Name`
- `Duration`
- `Color`
- `NotifyClassName`
- `Payload`

붙여 넣을 때 시간과 `StableId`는 새로 정해집니다.

## 10. Type과 Notify Class를 어떻게 고를까

먼저 `Type`을 정하고, 그 다음에 맞는 `Notify Class`를 고르는 방식으로 생각하면 쉽습니다.

### 10.1 `Type = Notify`일 때

한 번 실행되는 클래스용입니다.

대표 예:

- `UAnimNotify_PlaySFX`
- `UAnimNotify_GameplayEvent`
- `UAnimNotify_CameraShake`
- `UAnimNotify_FootstepSurfaceEvent`
- `UAnimNotify_PlayVFX`
- `UAnimNotify_SpawnDecal`
- `UAnimNotifyLog`

### 10.2 `Type = Notify State`일 때

시작/종료 구간이 필요한 클래스용입니다.

대표 예:

- `UAnimNotifyState_GameplayEventWindow`
- `UAnimNotifyState_PlayLoopingSFX`
- `UAnimNotifyState_AttackWindow`
- `UAnimNotifyStateLog`

### 10.3 잘못 고르면 어떻게 되나

예를 들어 `Type`은 `Notify`인데 `Notify State` 클래스를 고르거나, 반대로 설정하면 `Validation`에서 오류가 납니다.

즉:

- `Notify`에는 `UAnimNotify` 계열 클래스
- `Notify State`에는 `UAnimNotifyState` 계열 클래스

를 사용해야 합니다.

## 11. 내장 AnimNotify 클래스 설명

아래는 코드에 등록된 대표 내장 클래스들입니다.

### 11.1 `UAnimNotify_PlaySFX`

한 번만 효과음을 재생합니다.

주요 Payload:

- `SoundCue` 필수
- `SocketName` 선택
- `VolumeMultiplier` 선택
- `Spatialized` 선택

예시:

```txt
SoundCue=Footstep_Stone;SocketName=foot_l;VolumeMultiplier=0.9;Spatialized=true
```

언제 쓰나:

- 발소리
- 피격 소리
- 무기 충돌음

### 11.2 `UAnimNotifyState_PlayLoopingSFX`

구간 동안 루프 사운드를 재생합니다.

주요 Payload:

- `SoundCue` 필수
- `SocketName` 선택
- `VolumeMultiplier` 선택
- `Spatialized` 선택

예시:

```txt
SoundCue=SwordHum;SocketName=weapon_tip;VolumeMultiplier=1.0;Spatialized=true
```

언제 쓰나:

- 지속되는 검 이펙트 사운드
- 차징 사운드

### 11.3 `UAnimNotify_GameplayEvent`

애니메이션 타이밍에 게임플레이 이벤트를 한 번 보냅니다.

주요 Payload:

- `EventName` 필수
- `Payload` 선택

예시:

```txt
EventName=Combo.Open;Payload=Light1
```

언제 쓰나:

- 콤보 입력 가능 시점 알림
- 특정 상태 전환 신호

### 11.4 `UAnimNotifyState_GameplayEventWindow`

구간 시작과 끝에서 게임플레이 이벤트를 보냅니다.

주요 Payload:

- `EventName` 필수
- `EndEventName` 선택
- `Payload` 선택

예시:

```txt
EventName=InvulnWindow;EndEventName=InvulnWindowEnd;Payload=Short
```

설명:

- 시작 시 `EventName`
- 종료 시 `EndEventName`
- `EndEventName`이 비어 있으면 종료 시에도 `EventName`을 재사용합니다.

언제 쓰나:

- 무적 구간
- 입력 허용 구간
- 방어 판정 구간

### 11.5 `UAnimNotify_CameraShake`

카메라 흔들림을 1회 실행합니다.

주요 Payload:

- `Shake` 필수
- `Scale` 선택

예시:

```txt
Shake=HeavyHit;Scale=0.75
```

언제 쓰나:

- 강한 타격
- 착지 충격
- 보스 연출

### 11.6 `UAnimNotify_PlayVFX`

VFX를 재생합니다.

주요 Payload:

- `Effect` 필수
- `SocketName` 선택
- `Attached` 선택
- `Scale` 선택

예시:

```txt
Effect=SlashArc;SocketName=weapon_tip;Attached=true;Scale=1.0
```

언제 쓰나:

- 검 궤적
- 손 이펙트
- 착지 먼지

### 11.7 `UAnimNotify_SpawnDecal`

데칼을 생성합니다.

주요 Payload:

- `Decal` 필수
- `SocketName` 선택
- `TraceDistance` 선택
- `Size` 선택
- `Lifetime` 선택
- `AlignToHitNormal` 선택

예시:

```txt
Decal=FootDust;SocketName=foot_l;TraceDistance=50.0;Size=1.0;Lifetime=2.0;AlignToHitNormal=true
```

언제 쓰나:

- 발자국
- 착지 자국
- 무기 충돌 흔적

### 11.8 `UAnimNotify_FootstepSurfaceEvent`

발 아래 표면을 추적해서 발소리 이벤트를 보냅니다.

주요 Payload:

- `EventName` 필수
- `SocketName` 필수
- `TraceDistance` 선택
- `Payload` 선택

예시:

```txt
EventName=Footstep;SocketName=foot_l;TraceDistance=25.0;Payload=Run
```

언제 쓰나:

- 왼발/오른발 발소리
- 표면별 발소리 분기

주의:

- 이 클래스는 `SocketName`이 사실상 꼭 필요합니다.

### 11.9 `UAnimNotifyState_AttackWindow`

공격 판정이 켜져 있는 구간을 나타냅니다.

주요 Payload:

- `ComponentName` 필수
- `AttackId` 선택
- `Priority` 선택

예시:

```txt
ComponentName=WeaponHitbox;AttackId=Light1;Priority=1
```

동작 개념:

- 시작 시 `ComponentName`에 해당하는 `PrimitiveComponent`의 overlap 이벤트를 활성화합니다.
- 종료 시 원래 상태로 복구합니다.
- `AttackId`가 있으면 소유 액터에 `AttackId:<값>` 태그를 붙였다가 끝날 때 제거합니다.

언제 쓰나:

- 공격 판정 on/off
- 연속 공격 구간 관리

주의:

- `ComponentName`은 실제 프리뷰 액터 안에 존재하는 컴포넌트 이름과 일치해야 합니다.

### 11.10 테스트용 클래스

#### `UAnimNotifyLog`

실행 시 로그를 남깁니다.

처음 연결 테스트할 때 매우 유용합니다.

#### `UAnimNotifyStateLog`

`Begin`, `Tick`, `End` 시점 로그를 남깁니다.

구간형 이벤트 디버깅에 좋습니다.

## 12. Payload 입력 방법

## 12.1 구조화된 편집기

일부 클래스는 `Payload`를 직접 문자열로 치지 않아도 됩니다.

`Notify Class`에 맞는 스키마가 있으면 에디터가 필드를 분리해서 보여 줍니다.

예:

- `Sound Cue`
- `Socket Name`
- `Volume Multiplier`
- `Spatialized`

이 방식이 가장 안전합니다.

이유:

- 키 이름 오타를 줄일 수 있습니다.
- 저장 시 canonical 형태로 정리됩니다.
- 검증 메시지도 더 정확하게 나옵니다.

## 12.2 Raw Payload 직접 입력

스키마가 없는 클래스거나 수동 수정이 필요하면 `Payload` 문자열을 직접 입력할 수 있습니다.

형식 예:

```txt
Key=Value;Key=Value;Key=Value
```

예:

```txt
EventName=Combo.Open;Payload=Light1
```

### 12.3 Advanced Raw Payload

구조화된 편집기를 쓰는 클래스도 `Advanced Raw Payload`를 펼치면 원문을 직접 볼 수 있습니다.

이 기능은 다음 상황에 유용합니다.

- 수동으로 세부 값 조정
- 기존 레거시 키 확인
- 복사해서 외부 문서에 남기기

## 13. Socket / Component 선택 필드 이해하기

일부 필드는 프리뷰 컨텍스트와 연결됩니다.

### 13.1 Socket Picker

예:

- `SocketName`

프리뷰 메시에 소켓이 있으면 콤보 박스로 선택할 수 있습니다.

프리뷰에서 선택 가능한 소켓이 없으면 수동 입력 필드로 보일 수 있습니다.

### 13.2 Component Picker

예:

- `ComponentName`

프리뷰 액터의 `PrimitiveComponent` 목록이 있으면 콤보 박스로 선택할 수 있습니다.

따라서 공격 판정용 `AttackWindow`를 만들 때는 프리뷰 대상 액터가 제대로 준비되어 있어야 편합니다.

## 14. Validation 보는 법

`Notify` 작업에서 가장 중요한 습관은 저장 전에 `Validation`을 확인하는 것입니다.

### 14.1 어디서 확인하나

2곳에서 볼 수 있습니다.

1. `Selected Notify > Validation`
2. 오른쪽 `Notify Debug > Validation`

### 14.2 어떤 종류가 나오나

- `Error`
- `Warning`
- `Info`

### 14.3 자주 나오는 문제

#### 1. 필수 Payload 누락

예:

- `PlaySFX`인데 `SoundCue` 없음
- `GameplayEvent`인데 `EventName` 없음
- `AttackWindow`인데 `ComponentName` 없음

#### 2. Type과 Class 불일치

예:

- `Notify`인데 `Notify State` 클래스 사용
- `Notify State`인데 일반 `Notify` 클래스 사용

#### 3. Notify State인데 Duration이 0 이하

구간형인데 길이가 없어서 경고가 납니다.

#### 4. 일반 Notify인데 Duration이 0보다 큼

일반 `Notify`는 한 번 실행용이므로 긴 duration은 의미가 맞지 않을 수 있습니다.

#### 5. 이름 없음

이름이 비어 있으면 식별이 불편하고 검증 메시지가 나올 수 있습니다.

#### 6. StableId 중복

문서 전체에서 `StableId`가 중복되면 오류가 납니다.

보통 정상적인 추가/복제 기능을 쓰면 자동으로 새 ID가 생성되므로 직접 데이터 편집을 하지 않는 한 드문 편입니다.

#### 7. 프리뷰 컨텍스트와 맞지 않는 값

예:

- 존재하지 않는 `SocketName`
- 존재하지 않는 `ComponentName`

### 14.4 Validation 목록 활용법

오른쪽 `Validation` 탭 목록에서 항목을 클릭하면 해당 `Notify`를 선택할 수 있습니다.

문제가 많은 시퀀스를 정리할 때 매우 유용합니다.

## 15. 처음 쓰는 사람을 위한 추천 작업 순서

아래 순서로 연습하면 가장 빠르게 익힐 수 있습니다.

### 15.1 1단계: 로그 Notify로 동작 확인

1. 빈 타이밍에 `Notify`를 추가한다.
2. `Notify Class`를 `UAnimNotifyLog`로 바꾼다.
3. `Name`을 `Test_Log`로 바꾼다.
4. 필요하면 `Payload=Hello`를 넣는다.
5. 재생하면서 로그를 확인한다.

이 단계는 "Notify가 실제로 호출되는지"를 가장 단순하게 검증하는 방법입니다.

### 15.2 2단계: 발소리 Notify 만들기

1. 왼발 닿는 프레임에 `Notify` 추가
2. `Notify Class = UAnimNotify_PlaySFX`
3. `SoundCue = Footstep_Stone`
4. `SocketName = foot_l`
5. `Spatialized = true`

오른발도 같은 방식으로 하나 더 만듭니다.

### 15.3 3단계: 공격 구간 Notify State 만들기

1. 공격이 유효한 시작 프레임에 `Notify State` 생성
2. 오른쪽 끝을 드래그해서 종료 프레임까지 늘림
3. `Notify Class = UAnimNotifyState_AttackWindow`
4. `ComponentName = WeaponHitbox`
5. 필요하면 `AttackId = Light1`

## 16. 실전 예시

## 16.1 예시 A: 기본 발소리

- 타입: `Notify`
- 클래스: `UAnimNotify_PlaySFX`
- 이름: `Footstep_L`
- 시간: 왼발 접지 시점
- Payload:

```txt
SoundCue=Footstep_Stone;SocketName=foot_l;VolumeMultiplier=1.0;Spatialized=true
```

## 16.2 예시 B: 콤보 입력 가능 구간

- 타입: `Notify State`
- 클래스: `UAnimNotifyState_GameplayEventWindow`
- 이름: `ComboWindow`
- Payload:

```txt
EventName=Combo.Open;EndEventName=Combo.Close;Payload=Light1
```

## 16.3 예시 C: 공격 히트박스 구간

- 타입: `Notify State`
- 클래스: `UAnimNotifyState_AttackWindow`
- 이름: `LightAttackWindow`
- Payload:

```txt
ComponentName=WeaponHitbox;AttackId=Light1;Priority=1
```

## 16.4 예시 D: 검 궤적 VFX

- 타입: `Notify`
- 클래스: `UAnimNotify_PlayVFX`
- 이름: `SlashTrail`
- Payload:

```txt
Effect=SlashArc;SocketName=weapon_tip;Attached=true;Scale=1.0
```

## 17. 자주 하는 실수

### 17.1 이름만 바꾸고 클래스는 기본값으로 두는 경우

`Name`은 표시 이름일 뿐입니다.
실제 동작은 `Notify Class`가 결정합니다.

### 17.2 Notify와 Notify State를 헷갈리는 경우

한 번만 실행할 것인지, 구간 전체에 영향을 줄 것인지 먼저 정해야 합니다.

### 17.3 Payload 키를 임의로 바꾸는 경우

예를 들어 `SoundCue`를 `Sound`로 쓰면 동작하지 않을 수 있습니다.
가능하면 구조화된 필드 편집기를 사용하세요.

### 17.4 없는 소켓 이름을 넣는 경우

특히 `SocketName`을 수동 입력할 때 자주 발생합니다.
가능하면 프리뷰에서 제공하는 선택 목록을 사용하세요.

### 17.5 AttackWindow에 잘못된 컴포넌트 이름을 넣는 경우

`ComponentName`은 정확히 일치해야 합니다.
오타가 나면 런타임에서 공격 판정이 켜지지 않습니다.

## 18. 체크리스트

저장 전에 아래만 확인해도 대부분의 실수를 줄일 수 있습니다.

- `Type`이 맞는가?
- `Notify Class`가 맞는가?
- `Name`이 기능을 설명하는가?
- 필수 `Payload` 키가 모두 있는가?
- `SocketName` 또는 `ComponentName`이 실제 프리뷰 대상과 일치하는가?
- `Notify State`라면 `Duration`이 충분한가?
- `Validation`에 `Error`가 없는가?

## 19. 요약

처음에는 아래 원칙만 기억하면 됩니다.

1. 한 번 실행이면 `Notify`, 구간이면 `Notify State`
2. 동작은 `Notify Class`가 결정한다
3. 세부 데이터는 `Payload`에 들어간다
4. 가능하면 구조화된 필드 편집기를 사용한다
5. 저장 전 `Validation`을 반드시 확인한다
6. 처음 테스트는 `UAnimNotifyLog`, `UAnimNotifyStateLog`로 시작하면 가장 쉽다

이 문서 기준으로 작업하면, 코드 수정 없이도 `Animation Sequence Viewer` 안에서 대부분의 기본 `AnimNotify` 설정과 검증을 진행할 수 있습니다.
