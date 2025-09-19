import cv2
import numpy as np
import time

cap = cv2.VideoCapture(1, cv2.CAP_DSHOW)

while True:
    ret, frame = cap.read()
    if not ret:
        break

    blurred = cv2.GaussianBlur(frame, (11, 11), 0)
    hsv = cv2.cvtColor(blurred, cv2.COLOR_BGR2HSV)

    # 파란색 범위 정의 (기존 코드로 복원)
    lower_blue = np.array([100, 150, 0])
    upper_blue = np.array([140, 255, 255])

    mask = cv2.inRange(hsv, lower_blue, upper_blue) # 'lower_blue', 'upper_blue' 사용
    kernel = np.ones((5, 5), np.uint8)
    opened = cv2.morphologyEx(mask, cv2.MORPH_OPEN, kernel)

    contours, _ = cv2.findContours(opened, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    
    is_object_detected = False

    for cnt in contours:
        if cv2.contourArea(cnt) > 500:
            is_object_detected = True
            
            x, y, w, h = cv2.boundingRect(cnt)
            cv2.rectangle(frame, (x, y), (x+w, y+h), (0, 255, 0), 2)

            M = cv2.moments(cnt)
            if M["m00"] != 0:
                cx = int(M["m10"] / M["m00"])
                cy = int(M["m01"] / M["m00"])
                cv2.circle(frame, (cx, cy), 5, (0, 0, 255), -1)
                cv2.putText(frame, f"Center: ({cx}, {cy})", (cx-50, cy-10),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 2)
            
            print(f"객체 감지: [좌표] x:{x}, y:{y} / [크기] 폭:{w}, 높이:{h}")
           

    if not is_object_detected:
        print("객체 미감지")
       

    cv2.imshow("Result", frame)
    cv2.imshow("Binary", opened)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break
    
    time.sleep(1)

cap.release()
cv2.destroyAllWindows()