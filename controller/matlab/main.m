clearvars

home_dir = char(java.lang.System.getProperty('user.home'));
scab_dir = fullfile(home_dir, "git", "scab-c");

min_nstims_begore_first_target = 4;
min_tt_distance = 2;
target = 1;
size_block = 5;
n_blocks = 5;

soa = 0.6;

Fs = 44100;
frames_per_buffer = 512;
%% generate stimulation plan

plan = generate_stimulation_plan(size_block, n_blocks, target, min_tt_distance, min_nstims_begore_first_target);
get_min_tt_distance(plan, target)

%% audio csv data
audio_csv_data = [];
for m = 1:length(plan)
    audio_csv_data = [audio_csv_data; soa*m];
    audio_csv_data = [audio_csv_data; 0];
    if rem((m-1), 3) == 0
        audio_csv_data = [audio_csv_data; 0];
    else
        
    end
end


%% functions
function r = get_min_tt_distance(plan, target)
I = find(plan == target);
r = min(diff(I));
end

function r = check_first_target(block, target)
r = find(block == target);
r = r(1);
end

function v = shuffle(v)
v = v(randperm(length(v)));
end

function plan = generate_stimulation_plan(size_block, n_blocks, target, min_tt_distance, min_nstims_begore_first_target)
block = 1:size_block;

while 1
    plan = shuffle(block);
    if check_first_target(plan, target) >= min_nstims_begore_first_target
        break
    end
end

for m = 2:n_blocks
    while 1
        block_ = shuffle(block);
        plan_ = [plan block_];
        if get_min_tt_distance(plan_, target) >= min_tt_distance
            plan = plan_;
            break
        end
    end
end
end

