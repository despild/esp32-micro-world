# 이퀄툴 웹 디자인

대상은 `web/index.html`, 작업 모드는 측정 도구(Operate)입니다. 사용자는 ESP32-C3에 Wi-Fi로 연결한 휴대폰에서 압력의 변화를 보고, 필요할 때 표시를 멈추거나 영점과 기준 압력을 교정합니다.

## 화면 원칙

- 파형이 주 콘텐츠입니다. 소개용 히어로, 반복 카드, 장식 아이콘, 그림자, 그라데이션, 진입 애니메이션을 사용하지 않습니다.
- 현재 측정값과 연결 상태를 위에 두고, 최근 20초 파형과 표시 조작을 이어 배치합니다. 영점·교정과 연결 안내는 기본적으로 접습니다.
- 밝은 중성 배경 `#f6f7f4`, 본문 `#202923`, 보조 글자 `#59645c`, 구분선 `#c4ccc5`를 사용합니다. 실제 파형은 `#235e47`, 샘플 파형은 `#995019`입니다. 샘플이라는 문구와 % 단위를 함께 표시해 색에만 의존하지 않습니다.
- 센서가 없는 상태에서는 현재 값이 `—`입니다. 샘플은 표시 전용이고 실제 측정값이나 권장 압력이 아닙니다. 장치와 연결이 끊기면 샘플 애니메이션도 중단합니다.
- 시스템 글꼴을 사용해 인터넷과 폰트 다운로드가 필요 없게 합니다. 측정 숫자는 tabular numerals, 버튼과 입력은 최소 44px 높이입니다. 키보드 포커스는 3px 파란 외곽선으로 표시합니다.
- 모바일에서는 상태와 설정 행을 세로로 배치합니다. 320px에서도 가로 스크롤이 없어야 하며, 그래프 높이는 최소 240px입니다. 데스크톱 콘텐츠 최대 폭은 1040px입니다.

## 구현과 검증

별도 프레임워크나 외부 CDN을 추가하지 않습니다. `web/index.html`이 원본이며 `tools/embed_web.py`로 OLED 샘플 파형과 gzip 웹 자산을 생성합니다. `page.h`를 직접 수정하지 않습니다.

`tools/test_web.cjs`는 모의 전송으로 상태 전환, CSV, 실제 버튼 명령 전송, 반응형 배치, 키보드 포커스를 검사합니다. `--serve`를 붙이면 테스트용 미리보기 서버를 시작합니다. 이 서버는 실제 보드가 아닙니다.

Impeccable의 [스킬 지침](https://github.com/pbakaus/impeccable/blob/main/.agents/skills/impeccable/SKILL.md), [Distill](https://github.com/pbakaus/impeccable/blob/main/.agents/skills/impeccable/reference/distill.md), [Operate](https://github.com/pbakaus/impeccable/blob/main/.agents/skills/impeccable/reference/operate.md)를 참고했습니다. 자동 검사는 HTML 소스와 실행 중인 미리보기 URL을 대상으로 하며, 결과만으로 사용성을 보장하지 않고 실제 렌더링도 확인합니다.

이 환경에서는 직접 URL 검사 시 지속적인 실시간 요청과 브라우저 시작 문제로 검사를 완료하지 못했습니다. `tools/check_design.cjs`는 Playwright로 미리보기를 열고, 공식 `impeccable detect --cdp`로 이미 표시된 화면에 연결합니다. `PLAYWRIGHT_MODULE`, `BROWSER_EXE`, `IMPECCABLE_ENGINE`에 설치된 도구 경로를 지정하고 미리보기 URL을 인수로 전달합니다.

2026-11-12 검사에서 HTML 소스와 390×844 화면은 지적 사항 0개였습니다. 1280×960 화면은 주요 지적 사항 0개, `flat-type-hierarchy` 참고 의견 1개였습니다. 14/16/18/22px의 조밀한 글자 크기는 조작 도구의 작은 제목과 레이블을 위한 선택이며, 굵기와 구분선으로 위계를 구분합니다. Impeccable의 Operate 지침도 이처럼 좁은 크기 비율을 허용하므로 참고 의견을 없애기 위한 제목 확대는 하지 않았습니다.
