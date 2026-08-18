추가 구현 사항

1. 각속도
- "Angular Velocity" 옵션을 키면 활성화
UBall 클래스 안에 AngularVelocity 변수를 랜덤하게 저장하고, Shader에서 회전을 계산하여 Ball이 각자 다른 속도로 회전할 수 있게 구현하였습니다.

2. 탄성
- "Restitution" 옵션을 키면 활성화
UBall 클래스 안에 Restitution 변수를 랜덤하게 저장하고, 벽 혹은 다른 Ball과 충돌 시 "비탄성 충돌"이 일어나도록 구현하였습니다. 

3. 충돌 시 색상 어두워짐
- "Darken on Hits" 옵션을 키면 활성화
옵션 활성화 시, UBall 클래스 안에서 NumHits 변수를 통해 다른 Ball과 충돌 횟수를 저장합니다. 충돌 횟수가 증가할 때마다 Ball의 색상이 어두워지도록 구현하였습니다.


