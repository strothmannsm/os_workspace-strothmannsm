# OS_Workspace

A repo of your very own!


## Remote Setup:

This will allow your workspace to download future updates and assignments.

	git remote add upstream git@github.com:mizzoucs/OSSP16_Assignments.git


## Remote Sync:

Check for updates often!
These updates will contain future assignments, milestones, example code, and bugfixes.
**_Keeping your repository up to date is your own responsibility._**

Updates can be downloaded with:

	git fetch -t upstream

To start working on a project/milestone, you will need to merge the relevent base code into your repository.
These code bases will be identified by their base tag (ex: `P1M1_BASE`) and can be merged into your repository by merging with said tag

	git merge <BASE_TAG>


## Managing Merge Conflicts

Merge conflicts happen, and they can get pretty nasty.
The `mergetool` module will launch tools to aid in the resolution of merge conflicts.
You can use `meld` to resolve merge conflicts with

	git mergetool --tool=meld

If `meld` isn't currently installed, you can install it with

	sudo apt-get install -y meld


## Submission:

Tags will be used to track and download your submissions for grading.
To submit, you will need to have the submission tag for the assignment/milestone (ex: `P1M1`).
The submission tag can be found on the assignment document or in the individual project readme.

Tags are neither automagically created nor uploaded to GitHub, so remember to create and submit yours before the due date!
Creating a tag is super easy, just run

	git tag -a <TAG> -m <MESSAGE>

to create a tag of your current commit, and push it along with your commits to github with

	git push --follow-tags origin

Submissions will be automatically collected some point after the deadline has passed.


## Updating Your Submission:

If you want to update your tagged version, first delete old tag with

	git tag -d <TAG>
	
If you have already pushed your tag to GitHub, you must also delete the uploaded tag with

	git push origin :<TAG>
	
After the existing tag has been deleted, you can retag the correct commit and push it to GitHub like before.


## OS_Library!

The library will recieve updates as needed throughout the semester.

	git clone git@github.com:mizzoucs/OS_Library.git
	cd OS_Library && mkdir build && cd build
	cmake .. && sudo make install && sudo ldconfig


## Additional Git Resources:

* [Pro Git](http://git-scm.com/book/en/v2)
* [Branching and Merging](https://git-scm.com/book/en/v2/Git-Branching-Basic-Branching-and-Merging)
* [Tagging](https://git-scm.com/book/en/v2/Git-Basics-Tagging)
* [Google](https://google.com)
