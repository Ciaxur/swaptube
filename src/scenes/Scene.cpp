#pragma once

#include <unordered_map>
#include <chrono>
#include <cassert>
#include "../misc/Dagger.cpp"
#include "../io/DebugPlot.h"
#include "../misc/pixels.h"
#include "../io/AudioSegment.cpp"

using namespace std;

static int frame_number;
static bool FOR_REAL = true; // Whether we should actually be writing any AV output
static bool PRINT_TO_TERMINAL = true;

class Scene {
public:
    virtual void draw() = 0;
    Scene(const int width = VIDEO_WIDTH, const int height = VIDEO_HEIGHT)
        : w(width), h(height), dag(make_shared<Dagger>()), pix(width, height) {
        dag->set_special("frame_number", 0);
        dag->set_special("audio_segment_number", 0);
        dag->set_special("transition_fraction", 0);
        dag->set_special("subscene_transition_fraction", 0);
        dag->add_equation("t", "<frame_number> " + to_string(VIDEO_FRAMERATE) + " /");
    }

    // Scenes which contain other scenes use this to populate the StateQuery
    virtual void pre_query(){}
    virtual bool scene_requests_rerender() const = 0;
    void query(Pixels*& p) {
        pre_query();
        update_state();
        if(state != last_state || scene_requests_rerender()) draw();
        p=&pix;
    }

    void resize(int width, int height){
        if(w == width && h == height) return;
        w = width;
        h = height;
        pix = Pixels(w, h);
    }

    void inject_audio_and_render(const AudioSegment& audio){
        inject_audio(audio, 1);
        render();
    }

    void inject_audio(const AudioSegment& audio, int expected_video_sessions){
        WRITER.add_shtooka(audio);
        if(!FOR_REAL)
            return;
        cout << "Scene says: " << audio.get_subtitle_text() << endl;
        if (video_sessions_left != 0) {
            failout("ERROR: Attempted to add audio without having finished rendering video!\nYou probably forgot to use render()!\n"
                    "This superscene was created with " + to_string(video_sessions_total) + " total video sessions, "
                    "but render() was only called " + to_string(video_sessions_total-video_sessions_left) + " times.");
        }

        superscene_frames_total = superscene_frames_left = WRITER.add_audio_segment(audio) * VIDEO_FRAMERATE;
        video_sessions_total = video_sessions_left = expected_video_sessions;
        cout << "Scene should last " << superscene_frames_left << " frames, with " << expected_video_sessions << " sessions.";
    }

    void render(){
        if(!FOR_REAL){
            dag->close_all_transitions();
            return;
        }

        int video_sessions_done = video_sessions_total - video_sessions_left;
        int superscene_frames_done = superscene_frames_total - superscene_frames_left;
        double num_frames_per_session = static_cast<double>(superscene_frames_total) / video_sessions_total;
        int num_frames_to_be_done_after_this_time = round(num_frames_per_session * (video_sessions_done + 1));
        scene_duration_frames = num_frames_to_be_done_after_this_time - superscene_frames_done;
        cout << "Rendering a scene. Frame Count: " << scene_duration_frames << " (sessions left: " << video_sessions_left << ", " << superscene_frames_left << " frames total)" << endl;

        for (int frame = 0; frame < scene_duration_frames; frame++) {
            render_one_frame(frame);
        }
        cout << endl;
        video_sessions_left--;
        if(video_sessions_left == 0){
            dag->close_all_transitions();
            dag->set_special("audio_segment_number", (*dag)["audio_segment_number"] + 1);
        }
    }

    void update_state() {
        last_state = state;
        state = dag->get_state(state_query);
    }

    Pixels* expose_pixels() {
        return &pix;
    }

    int w = 0;
    int h = 0;
    shared_ptr<Dagger> dag;
    StateQuery state_query;

private:
    void render_one_frame(int subscene_frame){
        auto start_time = chrono::high_resolution_clock::now(); // Start timing

        dag->set_special("frame_number", frame_number);
        frame_number++;
        dag->set_special("transition_fraction", 1 - static_cast<double>(superscene_frames_left) / superscene_frames_total);
        dag->set_special("subscene_transition_fraction", static_cast<double>(subscene_frame) / scene_duration_frames);

        dag->evaluate_all();
        dag_time_plot.add_datapoint(vector<double>{(*dag)["t"], (*dag)["transition_fraction"], (*dag)["subscene_transition_fraction"]});

        if (video_sessions_left == 0) {
            failout("ERROR: Attempted to render video, without having added audio first!\nYou probably forgot to inject_audio() or inject_audio_and_render()!");
        }

        Pixels* p = nullptr;
        WRITER.set_time((*dag)["t"]);
        query(p);
        if(PRINT_TO_TERMINAL && (int((*dag)["frame_number"]) % 5 == 0)) p->print_to_terminal();
        WRITER.add_frame(*p);
        superscene_frames_left--;

        auto end_time = chrono::high_resolution_clock::now(); // End timing
        chrono::duration<double, milli> frame_duration = end_time - start_time; // Calculate duration in milliseconds
        time_per_frame_plot.add_datapoint(frame_duration.count()); // Add the time to DebugPlot
        memutil_plot.add_datapoint(get_free_memory()); // Add the time to DebugPlot
        cout << "#";
        fflush(stdout);
    }

protected:
    void append_to_state_query(StateQuery sq){
        state_query.insert(sq.begin(), sq.end());
    }
    bool rendered = false;
    bool is_transition = false;
    Pixels pix;
    int video_sessions_left = 0;
    int video_sessions_total = 0;
    int scene_duration_frames = 0;
    int superscene_frames_left = 0;
    int superscene_frames_total = 0;
    State state;
    State last_state;
};
