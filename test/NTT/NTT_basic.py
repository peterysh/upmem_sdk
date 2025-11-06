import numpy as np

def butterfly(p, q, w):
    """
    C 코드의 버터플라이 연산을 int32 타입으로 정확히 시뮬레이션합니다.
    """
    # np.int64로 중간 계산을 수행하여 파이썬의 자동 승격(promotion)을 방지하고
    # C와 동일한 int32 오버플로우를 흉내 냅니다.
    p_long = np.int64(p)
    q_long = np.int64(q)
    w_long = np.int64(w)
    
    result_p = p_long + q_long * w_long
    result_q = p_long - q_long * w_long
    
    # C의 int32_t처럼 32비트로 자른(오버플로우된) 결과를 반환합니다.
    return np.int32(result_p), np.int32(result_q)

# --- 1. 상수 정의 ---
BUFFER_SIZE = 256

# --- 2. 메모리 초기화 ---
# C 코드의 init_point_array + mram_read 와 동일합니다.
# 1부터 256까지 채워진 int32 배열을 생성합니다.
read_cache = np.array(range(1, BUFFER_SIZE + 1), dtype=np.int32)

print("--- 초기 상태 (init_point_array + mram_read 완료) ---")
# C 코드의 첫 번째 printf 루프와 비교 (C 코드는 %u, 여기서는 %d로 출력)
for i in range(BUFFER_SIZE):
    if i < 8 or i > BUFFER_SIZE - 9: # 너무 길어 앞/뒤만 출력
        print(f"read_cache[{i:3}] = {read_cache[i]}")
print("... (중략) ...\n")


# --- 3. Stage 루프 실행 (C 코드의 main 루프) ---
stage = 1
# C 코드의 루프 조건: stage * stage < BUFFER_SIZE (stage < 16)
# stage는 1, 2, 4, 8 이 됩니다.
while stage * stage < BUFFER_SIZE:
    print(f"--- Stage {stage} 처리 중 ---")
    
    # 트위들 팩터 (C 코드에서 1로 고정됨)
    w = np.int32(1)
    
    # C 코드의 내부 루프 (0 ~ 127)
    for i in range(BUFFER_SIZE // 2):
        
        # C 코드의 인덱싱 로직
        index = (i // stage) * (2 * stage) + (i % stage)
        top = index
        bottom = index + stage
        
        # C 코드의 버터플라이 연산
        p = read_cache[top]
        q = read_cache[bottom]
        
        result_p, result_q = butterfly(p, q, w)
        
        # C 코드와 동일하게 "in-place" (원본 배열)에 덮어씁니다.
        read_cache[top] = result_p
        read_cache[bottom] = result_q
            
    # 다음 스테이지로
    stage = stage << 1

# --- 4. 최종 결과 출력 ---
print("\n--- 최종 결과 (모든 Stage 완료) ---")
print("이 결과가 C 코드의 Tasklet 0번이 마지막에 출력하는 '------Result-------'와 일치해야 합니다.")
for i in range(BUFFER_SIZE):
    # C 코드의 최종 printf(%d)와 비교
    print(f"result[{i:3}] = {read_cache[i]}")