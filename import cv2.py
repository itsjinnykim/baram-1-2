import cv2
import numpy as np
import serial
import time

# --- 시리얼 통신 설정 ---
try:
    ser = serial.Serial('COM3', 9600, timeout=1) 
    print("시리얼 포트 연결 성공")
except serial.SerialException as e:
    print(f"시리얼 포트 연결 실패: {e}")
    ser = None

if ser:
    time.sleep(2)

# --- 기존 OpenCV 코드 ---
cap = cv2.VideoCapture(1, cv2.CAP_DSHOW)

while True:
    ret, frame = cap.read()
    if not ret:
        break

    blurred = cv2.GaussianBlur(frame, (11, 11), 0)
    hsv = cv2.cvtColor(blurred, cv2.COLOR_BGR2HSV)

    # 노란색~초록색 범위 정의
    lower_color = np.array([25, 50, 70])
    upper_color = np.array([89, 255, 255])

    mask = cv2.inRange(hsv, lower_color, upper_color)
    kernel = np.ones((5, 5), np.uint8)
    opened = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)

    contours, _ = cv2.findContours(opened, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    
    # 전송할 데이터 초기화
    data_to_send = ""
    is_object_detected = False # 객체 감지 여부 플래그

    for cnt in contours:
        if cv2.contourArea(cnt) > 500:
            is_object_detected = True
            
            # 경계 사각형
            x, y, w, h = cv2.boundingRect(cnt)
            cv2.rectangle(frame, (x, y), (x+w, y+h), (0, 255, 0), 2)

            # 무게중심 (Centroid)
            M = cv2.moments(cnt)
            if M["m00"] != 0:
                cx = int(M["m10"] / M["m00"])
                cy = int(M["m01"] / M["m00"])
                cv2.circle(frame, (cx, cy), 5, (0, 0, 255), -1)
                cv2.putText(frame, f"Center: ({cx}, {cy})", (cx-50, cy-10),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 2)
                
                # 시리얼 전송을 위한 데이터 포맷팅
                # 'cx,cy,w,h\n' 형태로 문자열 생성
                data_to_send = f"{cx},{cy},{w},{h}\n"

    # 시리얼 데이터 전송
    if ser:
        if is_object_detected and data_to_send:
            # 객체가 감지되었을 때만 데이터 전송
            ser.write(data_to_send.encode('utf-8'))
            print(f"데이터 전송: {data_to_send.strip()}")
        else:
            # 객체가 감지되지 않았을 때 '0,0,0,0\n' 전송 (ATmega가 객체 없음 상태를 알 수 있도록)
            ser.write("0,0,0,0\n".encode('utf-8'))
            print("객체 미감지: 0,0,0,0 전송")

    cv2.imshow("Result", frame)
    cv2.imshow("Binary", opened)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# --- 종료 시 시리얼 포트 닫기 ---
cap.release()
cv2.destroyAllWindows()
if ser:
    ser.close()
    print("시리얼 포트 연결 종료")