# TBS Simulator

## Build

- Linux 환경에서 make를 이용하여 build, gcc 필요
- \# make

## Run
- ./runtbs &lt;options&gt; &lt;config path&gt;
  - -h 옵션을 주면 간단한 설명 표시됨
  - -p 옵션을 이용하여 정책을 지정할 수 있음
  - -v : verbose mode
  - -g : kernel workload 자동 생성 모드, 새로운 설정 파일 경로를 인자로 제공해야 함
- simtbs.conf.tmpl을 참고하여 configuration file 생성
- 현재 지원되는 스케쥴링 정책은 rr, rrf, bfa, dfa 4가지

## Configuration File Syntax
- #으로 시작하면 해당 행은 무시
- 첫번째 컬럼이 *로 시작하면 section 명
  - section은 현재 general, workload, sm, overhead, kernel
- general section은 일반적 설정 정보. 현재는 최대 시뮬레이션 수행 시간만 설정 가능
  - workload 자동 생성 모드의 경우 반드시 최대 수행 시간을 지정해야 함
- workload section은 워크로드 자동 생성시 필요한 설정 정보
  - level: 워크로드를 발생시키는 최소 SM 사용율(1~100)
  - max\_starved: TB를 수행시키지 않은 starved 상태의 커널의 최대 개수, max\_starved가 1이하인 경우는 starved kernel이 없는 경우에만 워크로드를 발생시킴
  - n\_tbs\_range: 커널의 TB 개수 범위
  - tb\_duration_range: 단위 TB의 수행 시간 범위
- sm section은 SM의 개수와 SM에서 지원가능한 최대 Resource 정의
- overhead section은 sm에서 사용중인 resource에 따라 TB의 추가 수행 시간정의
  - 1이하로는 정의할 수 없음, 1의 경우는 overhead가 없음
  - 3 0.1로 정의된 경우는 SM에 2나 3의 resource가 사용중인 경우 10% 추가 overhead가 소요
  - 최대 resource에 대한 overhead를 반드시 정의해야 함
- kernel section은 simulation에 제공되는 kernel을 정의
  - 시작 timestamp, 커널에 정의된 TB개수, 1개 TB의 resource 사용량, TB의 수행시간(in timestamp)

## 스케쥴링 정책
### rr(Round Robin) ###
- 순차적으로 TB를 SM에 할당

### rrf(Round Robin Fully) ###
- rr과 동일하게 동작하나 SM을 모두 채운 후 다음 SM에 대해 할당

### bfa(Breadth First Allocation) ###
- 너비 우선으로 TB를 할당
- 가장 Resource를 작게 쓰는 SM부터 우선하여 TB를 할당
- 전체적으로 SM의 자원 사용율을 비슷하게 유지

### dfa(Depth First Allocation) ###
- 깊이 우선으로 TB를 할당
- 가장 Resource를 많이 쓰는 SM부터 우선하여 TB를 할당

