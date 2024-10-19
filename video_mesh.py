import pyray as rl
from pyray import KeyboardKey as K
import cv2
import time



def get_image_from_frame(frame: cv2.Mat) -> rl.Image:
    height = len(frame)
    width = len(frame[0])
    frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    image = rl.Image(frame.tobytes(),width, height, 1, rl.PixelFormat.PIXELFORMAT_UNCOMPRESSED_R8G8B8)
    return image

def main():
    WIDTH = 200
    HEIGHT = 150
    CAM_FPS = 30
    rl.init_window(1280, 720, "THING")
    video = cv2.VideoCapture(0)
    rl.set_target_fps(144)

    max_height = 50

    camera: rl.Camera3D = rl.Camera3D(rl.Vector3(0,3*max_height,0), rl.Vector3(0,max_height/2,-1), 
                rl.Vector3(0, 1, 0), 60.0, rl.CameraProjection.CAMERA_PERSPECTIVE)
    rl.disable_cursor()
    
    DUDE_SPEED = 30
    # MAIN LOOP
    while(not rl.window_should_close()):
        # Move
        dt = rl.get_frame_time()
        if (rl.is_key_down(K.KEY_LEFT_SHIFT)):
            speed = DUDE_SPEED*4
        else:
            speed = DUDE_SPEED
        
        rot = rl.Vector3(rl.get_mouse_delta().x*0.1, rl.get_mouse_delta().y*0.1, 0)
        
        dr = rl.Vector3((rl.is_key_down(K.KEY_W) - rl.is_key_down(K.KEY_S)) * speed * dt,
                      (rl.is_key_down(K.KEY_D) - rl.is_key_down(K.KEY_A)) * speed * dt, 0)
        rl.update_camera_pro(camera, dr, rot, 0)

        if (rl.is_key_down(K.KEY_SPACE)):
            camera.position.y += speed*dt
            camera.target.y += speed*dt
        if (rl.is_key_down(K.KEY_LEFT_CONTROL)):
            camera.position.y -= speed*dt
            camera.target.y -= speed*dt
        
        # Capture
        current_time = time.time()
        #if current_time - last_capture >= (1/CAM_FPS):
        ret, frame = video.read()
        if not ret:
            print("WHAT")
            exit(1)
        frame = cv2.resize(frame, (WIDTH, HEIGHT))
        ray_image = get_image_from_frame(frame) 
        last_capture = time.time()
        # Mesh

        model = rl.load_model_from_mesh(
                rl.gen_mesh_heightmap(ray_image, rl.Vector3(WIDTH, max_height, WIDTH)))
        texture = rl.load_texture_from_image(ray_image)
        model.materials[0].maps[rl.MaterialMapIndex.MATERIAL_MAP_ALBEDO].texture = texture
        
        # ---Drawing---
        rl.begin_drawing()
        rl.clear_background(rl.RAYWHITE)
        
        rl.begin_mode_3d(camera)
        
        rl.draw_model(model, rl.Vector3(-WIDTH/2, 0, -HEIGHT/2), 1, rl.WHITE)
        
        rl.end_mode_3d()
        
        # rl.draw_texture_ex(rl.load_texture_from_image(ray_image), rl.Vector2(0, 0),0, 2, rl.WHITE)
        rl.draw_fps(10, 10)
        text = f"{camera.position.y}, {camera.position.z}"
        rl.draw_text(text, 10, 30, 20, rl.BLACK)
        rl.end_drawing()
        # ---------------
        
    rl.close_window()

if __name__ == "__main__":
    main()