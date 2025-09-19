import cv2
import numpy as np
import time
import serial

try:
    ser = serial.Serial('COM3', 9600, timeout=1) 
    print("시리얼 포트 연결 성공")
    time.sleep(2)
except serial.SerialException as e:
    print(f"시리얼 포트 연결 실패: {e}")
    ser = None

cap = cv2.VideoCapture(1, cv2.CAP_DSHOW)

while True:
    ret, frame = cap.read()
    if not ret:
        break

    blurred = cv2.GaussianBlur(frame, (11, 11), 0)
    hsv = cv2.cvtColor(blurred, cv2.COLOR_BGR2HSV)

    # 빨간색
    lower_red1 = np.array([0, 150, 100])  # 명도를 0에서 100으로 변경
    upper_red1 = np.array([10, 255, 255])
    lower_red2 = np.array([170, 150, 100]) # 명도를 0에서 100으로 변경
    upper_red2 = np.array([180, 255, 255])

    # 마스크 결합 
    mask1 = cv2.inRange(hsv, lower_red1, upper_red1)
    mask2 = cv2.inRange(hsv, lower_red2, upper_red2)
    mask = cv2.bitwise_or(mask1, mask2) 
    
    kernel = np.ones((5, 5), np.uint8)
    opened = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)

    contours, _ = cv2.findContours(opened, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    
    object_detected = False
    data_to_send = ""

    if contours:
        largest_contour = max(contours, key=cv2.contourArea)
        if cv2.contourArea(largest_contour) > 500:
            object_detected = True
            
            x, y, w, h = cv2.boundingRect(largest_contour)
            cv2.rectangle(frame, (x, y), (x+w, y+h), (0, 255, 0), 2)
            
            M = cv2.moments(largest_contour)
            if M["m00"] != 0:
                cx = int(M["m10"] / M["m00"])
                cy = int(M["m01"] / M["m00"])
                cv2.circle(frame, (cx, cy), 5, (0, 0, 255), -1)
                cv2.putText(frame, f"Center: ({cx}, {cy})", (cx-50, cy-10),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 2)
            
            data_to_send = f"{x},{y},{w},{h}\n"
            print(f"객체 감지: [좌표] x:{x}, y:{y} / [크기] 폭:{w}, 높이:{h}")

    if ser:
        if object_detected and data_to_send:
            ser.write(data_to_send.encode('utf-8'))
        else:
            ser.write("0,0,0,0\n".encode('utf-8'))
            print("객체 미감지: 0,0,0,0 전송")

    cv2.imshow("Result", frame)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break
    
    time.sleep(0.1)

cap.release()
cv2.destroyAllWindows()
if ser:
    ser.close()
    print("시리얼 포트 연결 종료")