import cv2
import numpy as np
import time
import serial

# --- 시리얼 통신 설정 ---
# 포트 이름과 통신 속도를 ATmega와 동일하게 맞춰주세요.
try:
    ser = serial.Serial('COM3', 9600, timeout=1) 
    print("시리얼 포트 연결 성공")
    time.sleep(2)  # ATmega 초기화를 위해 잠시 대기
except serial.SerialException as e:
    print(f"시리얼 포트 연결 실패: {e}")
    ser = None

# --- OpenCV 코드 ---
cap = cv2.VideoCapture(1, cv2.CAP_DSHOW)

while True:
    ret, frame = cap.read()
    if not ret:
        break

    blurred = cv2.GaussianBlur(frame, (11, 11), 0)
    hsv = cv2.cvtColor(blurred, cv2.COLOR_BGR2HSV)

    # 파란색 범위 정의
    lower_blue = np.array([100, 150, 0])
    upper_blue = np.array([140, 255, 255])

    mask = cv2.inRange(hsv, lower_blue, upper_blue)
    kernel = np.ones((5, 5), np.uint8)
    opened = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)

    contours, _ = cv2.findContours(opened, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    
    is_object_detected = False
    data_to_send = ""

    # 가장 큰 객체를 찾아서 정보 전송 (여러 객체 감지 시)
    if contours:
        largest_contour = max(contours, key=cv2.contourArea)
        if cv2.contourArea(largest_contour) > 500:
            is_object_detected = True
            
            # 경계 사각형의 x, y, 폭, 높이
            x, y, w, h = cv2.boundingRect(largest_contour)
            
            # 화면에 사각형 그리기
            cv2.rectangle(frame, (x, y), (x+w, y+h), (0, 255, 0), 2)
            
            # 무게중심 계산 및 표시
            M = cv2.moments(largest_contour)
            if M["m00"] != 0:
                cx = int(M["m10"] / M["m00"])
                cy = int(M["m01"] / M["m00"])
                cv2.circle(frame, (cx, cy), 5, (0, 0, 255), -1)
                cv2.putText(frame, f"Center: ({cx}, {cy})", (cx-50, cy-10),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 2)
            
            # 시리얼 전송 데이터 포맷: "x,y,w,h\n"
            data_to_send = f"{x},{y},{w},{h}\n"
            print(f"객체 감지: [좌표] x:{x}, y:{y} / [크기] 폭:{w}, 높이:{h}")

    # 시리얼 데이터 전송
    if ser:
        if is_object_detected and data_to_send:
            # 객체가 감지되었을 때만 데이터 전송
            ser.write(data_to_send.encode('utf-8'))
        else:
            # 객체 미감지 시 '0,0,0,0' 전송
            ser.write("0,0,0,0\n".encode('utf-8'))
            print("객체 미감지: 0,0,0,0 전송")

    cv2.imshow("Result", frame)
    cv2.imshow("Binary", opened)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break
    
    # 각 루프 사이의 딜레이
    time.sleep(1)

# --- 종료 처리 ---
cap.release()
cv2.destroyAllWindows()
if ser:
    ser.close()
    print("시리얼 포트 연결 종료")