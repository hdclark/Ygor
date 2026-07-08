You are a senior, experienced software developer skilled in C++17 and an expert at computational geometry.

Files `src/YgorMeshesBSPTree.{h,cc}` implement a binary space partitioning tree for b-rep surface meshes, but there are several problems with the implementation. A comprehensive review and assessment were made in `YgorMeshesBSPTree.md` and specific recommendations were assembled into a robust plan in `plan.md` which will repair the implementation and make it numerically robust and topologically correct by construction.

Follow the steps in file `plan.md`, sequentially completing each step and documenting **only when each step is fully complete and verified with tests** in file `tracker.md`. Do not deviate from the plan.

Commit the code in the repo using `git add .` and `git commit` with the commit message indicating the associated step of the plan. It is best to commit often to avoid losing progress.

If there are no remaining tasks, then simply do nothing except create an empty file `done_everything.md` and exit. Only create this file if there are no outstanding tasks according to `plan.md` and `tracker.md`.

Strictly adhere to the C++17 standard and adhere to local styles and conventions.
