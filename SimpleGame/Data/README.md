# 게임 콘텐츠 데이터 v1

게임 작업 디렉터리를 `SimpleGame`으로 두면 시작 시 `Data/map.gse`, `Data/monsters.gse`, `Data/quests.gse`를 읽는다.
`Data/story.gse`가 있으면 튜토리얼 이후의 장별 이야기도 읽는다. 이 파일이 없는 기존 콘텐츠는 반복 사냥 흐름을 유지한다.
UTF-8 텍스트이며 한 줄에 한 레코드를 쓴다. 빈 줄과 `#`로 시작하는 주석 줄을 허용한다.
표시 문구는 큰따옴표로 감싼다. 따옴표와 역슬래시는 역슬래시로 이스케이프한다. 여러 줄 문구는 지원하지 않는다.
ID는 대소문자를 구분하며 영문·숫자·밑줄·하이픈·마침표로 최대 64자까지 쓴다.
좌표는 기존 월드의 X/Z 단위이다. 실행 중 수정은 반영하지 않으며 게임을 다시 시작해야 한다.

## 맵: map.gse

```text
gse_map 1 map_id
bounds minimum maximum
spawn x z
lake x z radius
notice_board x z
quest_point elder|tracks|herb x z
encounter monster_id x z
house id x z road_bend_x road_bend_z road_junction_z collision_half_x collision_half_z
tree id x z broadleaf|pine variant collision_radius
landmark id boulder|bush|log|fence|cart|ruin|pine x z display_height collision_radius
campfire id x z
npc id x z column row phase
animal id x z home_x home_z column row phase speed
```

맵의 배치 ID는 종류가 달라도 중복할 수 없다. 필수 단일 레코드와 세 quest_point도 중복할 수 없다.
집·모닥불은 각각 최대 32개, 나무·지형지물은 각각 512개, NPC·동물은 각각 128개이다.
나무 variant는 0~3, NPC column은 0~3/row는 0~1, 동물 column은 0~2/row는 2이다.
집과 나무의 충돌 크기는 플레이어 여유까지 포함한다. 지형지물은 radius가 0이면 통과하며 양수일 때 플레이어 여유 0.18을 더한다.
집·나무·지형지물은 같은 데이터로 그리기와 플레이어 이동 차단을 처리한다.

## 몬스터: monsters.gse

```text
gse_monsters 1
monster id "이름" health hit_range drop_range chase_range attack_range speed damage cooldown experience hide_drop tusk_drop
appearance monster_id sprite_height
```

템플릿은 최대 128개이다. 맵의 encounter가 사용할 템플릿 ID 하나를 선택한다.
기본 `lake_boar` 외에 `forest_boar` 예시가 있다. encounter의 ID를 바꾸면 선택한 능력치·표시 이름·보상을 사용한다.
이야기 전용 `rampaging_boar`, `displaced_boar`도 포함한다. 이야기 hunt 단계가 해당 템플릿을 선택한다.
모든 템플릿은 기존 멧돼지 이미지와 추적/공격 행동을 사용한다. 새로운 행동·모델을 자동 생성하는 형식은 아니다.
appearance는 앞서 선언한 템플릿마다 최대 한 번 쓴다. 시트 높이는 60~300이며 생략하면 125이다.

## 퀘스트: quests.gse

```text
gse_quests 1 quest_id
start stage_id "시작 알림"
repeat health_step maximum_bonus "반복 목표" "반복 힌트" "반복 수락 알림"
dialogue "장로 힌트" "장로 대화" "주민 대화"
stage id elder|tracks|herb|hunt|trophy|return|complete next_id "목표" "힌트" "처리 알림"
```

각 역할은 정확히 한 번 정의한다. 시작 역할은 elder/tracks/herb/hunt 중 하나이다.
앞의 세 역할은 순환 없이 hunt에 도달해야 한다. 사냥 이후 연결은 hunt/trophy/return/complete/hunt 순서를 유지한다.
이는 기존 행동을 데이터로 구성하는 형식이다. 호위·분기 선택 등 새 행동에는 코드 확장이 필요하다.

## 장별 이야기: story.gse

```text
gse_story 1 story_id
intro "수락 알림"
ending "마지막 완료 알림"
chapter id "장 제목"
region id "지역 이름" minimum_x minimum_z maximum_x maximum_z red green blue
step id chapter_id travel|investigate|hunt|collect|report|craft_armor x z radius monster_id|none "목표" "힌트" "완료 알림"
```

장·지역·목표 ID는 서로 중복하지 않는다. 장은 최대 16개, 지역은 최대 16개, 목표는 최대 128개이다.
intro·ending은 각각 필수 한 번이며, 장과 목표는 최소 하나가 필요하다.
목표는 파일 순서대로 진행하며 앞서 선언한 장만 참조한다. 이전 장으로 되돌아가는 순서는 거절한다.
목표점은 맵 안의 장애물·호수 밖이어야 하며 반경은 0.1~10이다. 지역 RGB는 0~1이다.
hunt만 몬스터 ID를 쓴다. 다른 행동은 none이다. hunt 다음에는 같은 장의 collect가 바로 이어져야 한다.
craft_armor는 장로에서 1.8 미만의 위치를 지정한다. 이전 사냥 보상만으로 가죽 2개·엄니 1개를 확보할 수 있어야 한다.
이야기는 첫 퀘스트 보고와 엄니 검 제작 이후 장로에게 E로 수락한다. 제작법 해금 후 R로 방어구를 제작한다.
지역 이름이 겹치면 나중에 정의한 지역을 표시한다. 색은 지면에 부드럽게 혼합한다.
현재 1·2장 14개 목표이며 원안의 전체 5장·분기·생태 행동을 구현한 것은 아니다.

## 오류 처리와 확인 범위

없는 파일·알 수 없는 버전/레코드·중복 ID·누락 필드·비정상 수치·잘못된 UTF-8·없는 참조를 거절한다.
단, 선택 파일 story.gse의 부재는 허용한다. 존재하면서 내용이 잘못된 경우는 거절한다.
시작점·퀘스트 지점·몬스터 출현점이 범위를 벗어나거나 장애물/호수 안에 있으면 거절한다.
각 파일은 256 KiB, 각 줄은 4096바이트, 문구는 512개의 UTF-16 코드 단위 이내이다.
전체 데이터를 검증한 뒤 한 번만 공유한다. 실패한 데이터를 부분 적용하거나 하드코딩 값으로 조용히 대체하지 않는다.

이번 단계는 소스·데이터 정적 점검만 수행했다. 로더 오류/확장 사례는 `tests/GameContentCheck.cpp`, 이야기 흐름 검사는 `tests/StoryFlowCheck.cpp`에 작성했으며 빌드·실행하지 않았다.
