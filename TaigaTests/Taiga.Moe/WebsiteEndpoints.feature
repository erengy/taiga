@Website
Feature: Website Endpoints

Test Website to see if it is available and the download works


Scenario: Website homepage is online and contains expected text
	When https://taiga.moe/ is requested
	Then The response code should be OK
	And The content should contain 'Taiga makes life easier for anime enthusiasts'

Scenario: Website download works
	When https://taiga.moe/download.php is requested
	Then The response code should be OK
	And The response type should be application/octet-stream