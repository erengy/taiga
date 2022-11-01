using TechTalk.SpecFlow;
using Xunit;

namespace TaigaTests.Taiga.Moe
{
    [Binding]
    public sealed class WebsiteEndpoints
    {
        HttpResponseMessage? response;
        private static HttpClient Client = new();
        [When("(.*) is requested")]
        public void URLIsRequested(string url)
        {
            HttpRequestMessage message = new()
            {
                RequestUri = new Uri(url),
                Method = HttpMethod.Get
            };
            response = Client.SendAsync(message).Result;
        }
        [Then("The response code should be (.*)")]
        public void ResponseCodeShouldBe(string code)
        {
            Assert.Equal(code, response?.StatusCode.ToString());
        }
        [Then("The response type should be (.*)")]
        public void ResponseTypeShouldBe(string type)
        {
            Assert.Equal(type, response?.Content?.Headers?.ContentType?.ToString());
        }
        [Then("The content should contain '(.*)'")]
        public void ContentShouldContain(string value)
        {
            Assert.True(response?.Content.ReadAsStringAsync().Result.Contains(value));
        }
    }
}