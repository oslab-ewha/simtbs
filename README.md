# TBS Simulator

## Build

- Linux 환경에서 make를 이용하여 build, gcc 필요
- \# make

## Run
- ./runtbs &lt;options&gt; &lt;config path&gt;
  - -h 옵션을 주면 간단한 설명 표시됨
  - -p 옵션을 이용하여 정책을 지정할 수 있음
- simtbs.conf.tmpl을 참고하여 configuration file 생성
- 현재 지원되는 스케쥴링 정책은 bfs, dfs 2가지

## Configuration Syntax
- #으로 시작하면 해당 행은 무시
- 첫번째 컬럼이 *로 시작하면 section 명
  - section은 현재 sm, overhead, kernel
- sm section은 SM의 개수와 SM에서 지원가능한 최대 Resource 정의
- overhead section은 sm에서 사용중인 resource에 따라 TB의 추가 수행 시간정의
  - 1이하로는 정의할 수 없음, 1의 경우는 overhead가 없음
  - 3 0.1로 정의된 경우는 SM에 2나 3의 resource가 사용중인 경우 10% 추가 overhead가 소요
  - 최대 resource에 대한 overhead를 반드시 정의해야 함
- kernel section은 simulation에 제공되는 kernel을 정의
  - 시작 timestamp, 커널에 정의된 TB개수, 1개 TB의 resource 사용량, TB의 수행시간(in timestamp)
