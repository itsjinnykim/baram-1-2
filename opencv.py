import cv2
import numpy as np
import time
import serial

# --- 트랙바 콜백 함수 ---
def nothing(x):
    pass

# --- 시리얼 포트 설정 ---
try:
    ser = serial.Serial('COM5', 9600, timeout=1)
    print("시리얼 포트 연결 성공")
    time.sleep(2)
except serial.SerialException as e:
    print(f"시리얼 포트 연결 실패: {e}")
    ser = None

# --- 웹캠 설정 ---
cap = cv2.VideoCapture(1, cv2.CAP_DSHOW)
if not cap.isOpened():
    print("카메라를 열 수 없습니다.")
    exit()

# --- 트랙바 윈도우 생성 및 초기값 설정 (요청값으로 변경됨) ---
cv2.namedWindow("HSV Range Tuner") 

# 초기값을 이미지에서 요청하신 값으로 설정합니다.
cv2.createTrackbar("H_MIN", "HSV Range Tuner", 37, 179, nothing)   # 37로 변경
cv2.createTrackbar("S_MIN", "HSV Range Tuner", 93, 255, nothing)  # 93으로 변경
cv2.createTrackbar("V_MIN", "HSV Range Tuner", 97, 255, nothing)  # 97로 변경
cv2.createTrackbar("H_MAX", "HSV Range Tuner", 11, 179, nothing)   # 11로 변경
cv2.createTrackbar("S_MAX", "HSV Range Tuner", 237, 255, nothing)  # 237로 변경
cv2.createTrackbar("V_MAX", "HSV Range Tuner", 255, 255, nothing) 

# H_LOW_2와 H_UP_2 트랙바는 제거되었습니다.


while True:
    ret, frame = cap.read()
    if not ret:
        print("프레임을 읽을 수 없습니다. 스트림을 종료합니다.")
        break

    # --- 트랙바에서 현재 HSV 값 읽기 (min/max 사용) ---
    h_min = cv2.getTrackbarPos("H_MIN", "HSV Range Tuner")
    s_min = cv2.getTrackbarPos("S_MIN", "HSV Range Tuner")
    v_min = cv2.getTrackbarPos("V_MIN", "HSV Range Tuner")
    h_max = cv2.getTrackbarPos("H_MAX", "HSV Range Tuner")
    s_max = cv2.getTrackbarPos("S_MAX", "HSV Range Tuner")
    v_max = cv2.getTrackbarPos("V_MAX", "HSV Range Tuner")
    
    # S, V는 MAX가 MIN보다 작아지지 않도록 보정 (H는 빨간색 감지를 위해 보정하지 않음)
    s_max = max(s_min, s_max)
    v_max = max(v_min, v_max)

    # --- HSV 임계값 설정 ---
    lower = np.array([h_min, s_min, v_min])
    upper = np.array([h_max, s_max, v_max])

    blurred = cv2.GaussianBlur(frame, (11, 11), 0)
    hsv = cv2.cvtColor(blurred, cv2.COLOR_BGR2HSV)

    # --- 마스크 생성 ---
    if h_min <= h_max:
        # 1. 일반적인 색상 감지 (H_MIN <= H_MAX)
        # 예: 노란색 (H_MIN=26, H_MAX=35)
        mask = cv2.inRange(hsv, lower, upper)
    else:
        # 2. 빨간색 감지 (H_MIN > H_MAX) - Hue 0을 가로지르는 경우
        # 예: 현재 요청값 H_MIN=37, H_MAX=11
        
        # H_MIN (37)부터 179까지의 범위
        lower_red_1 = np.array([h_min, s_min, v_min])
        upper_red_1 = np.array([179, s_max, v_max])
        mask1 = cv2.inRange(hsv, lower_red_1, upper_red_1)
        
        # 0부터 H_MAX (11)까지의 범위
        lower_red_2 = np.array([0, s_min, v_min])
        upper_red_2 = np.array([h_max, s_max, v_max])
        mask2 = cv2.inRange(hsv, lower_red_2, upper_red_2)
        
        mask = cv2.bitwise_or(mask1, mask2)


    # --- 형태학적 연산 ---
    kernel = np.ones((5, 5), np.uint8)
    opened = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)
    closed = cv2.morphologyEx(opened, cv2.MORPH_CLOSE, kernel)

    # --- 객체 찾기 ---
    contours, _ = cv2.findContours(closed, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    object_detected = False
    data_to_send = ""
    
    # 현재 HSV 범위 표시
    current_hsv_text = f"H:{h_min}-{h_max} / S:{s_min}-{s_max} / V:{v_min}-{v_max}"
    cv2.putText(frame, current_hsv_text, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)


    if contours:
        largest_contour = max(contours, key=cv2.contourArea)
        
        if cv2.contourArea(largest_contour) > 500:
            object_detected = True

            x, y, w, h = cv2.boundingRect(largest_contour)
            cv2.rectangle(frame, (x, y), (x+w, y+h), (0, 255, 0), 2) 

            # 무게 중심 계산 및 표시
            M = cv2.moments(largest_contour)
            if M["m00"] != 0:
                cx = int(M["m10"] / M["m00"])
                cy = int(M["m01"] / M["m00"])
                cv2.circle(frame, (cx, cy), 5, (0, 0, 255), -1)
                cv2.putText(frame, f"Center: ({cx}, {cy})", (cx-50, cy+20),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 2)

            data_to_send = f"{x},{y},{w},{h}\n"
            print(f"객체 감지 성공: [좌표] x:{x}, y:{y} / [크기] 폭:{w}, 높:{h}")


    # 시리얼 통신
    if ser:
        if object_detected and data_to_send:
            ser.write(data_to_send.encode('utf-8'))
        else:
            ser.write("0,0,0,0\n".encode('utf-8'))
            print("객체 미감지: 0,0,0,0 전송")

    cv2.imshow("Result", frame)
    cv2.imshow("Masked Output (Binary)", closed)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()
if ser:
    ser.close()
    print("시리얼 포트 연결 종료")